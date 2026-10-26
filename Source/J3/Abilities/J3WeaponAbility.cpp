// Copyright (c) ACCENT. All Rights Reserved.

#include "Abilities/J3WeaponAbility.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/J3CombatEffects.h"
#include "Abilities/J3GameplayTags.h"
#include "Core/J3PlayerState.h"
#include "Data/J3DataSubsystem.h"
#include "Engine/World.h"
#include "Inventory/J3InventoryComponent.h"
#include "Player/J3Character.h"

DEFINE_LOG_CATEGORY_STATIC(LogJ3Combat, Log, All);


namespace J3WeaponAbilityPrivate
{
	UJ3InventoryComponent* GetInventory(const FGameplayAbilityActorInfo* ActorInfo)
	{
		const AJ3PlayerState* PlayerState = ActorInfo != nullptr ? Cast<AJ3PlayerState>(ActorInfo->OwnerActor.Get()) : nullptr;
		return PlayerState != nullptr ? PlayerState->GetInventory() : nullptr;
	}
}


UJ3WeaponAbility::UJ3WeaponAbility()
{
	// 본인 화면은 바로 반응하고(예측), 피해 판정은 서버가 확정한다
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	ActivationBlockedTags.AddTag(J3Tags::State_Dead);
}


const FJ3EquipmentDef* UJ3WeaponAbility::GetWeaponDef(const FGameplayAbilityActorInfo* ActorInfo) const
{
	// 이 능력은 손에 든 장비에 맞춰 부여되므로, 손에 든 아이템의 행이 곧 이 무기의 수치다
	const UJ3InventoryComponent* Inventory = J3WeaponAbilityPrivate::GetInventory(ActorInfo);
	return Inventory != nullptr ? UJ3DataSubsystem::Get()->GetEquipment(Inventory->GetHeldItemId()) : nullptr;
}


const FGameplayTagContainer* UJ3WeaponAbility::GetCooldownTags() const
{
	static FGameplayTagContainer CooldownTags(J3Tags::Cooldown_Weapon);
	return &CooldownTags;
}


void UJ3WeaponAbility::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	// 빈 칸이면 config 기본값
	const FJ3EquipmentDef* Weapon = GetWeaponDef(ActorInfo);
	float CooldownSec = Weapon != nullptr ? Weapon->CooldownSec : -1.f;
	if (CooldownSec < 0.f)
	{
		CooldownSec = UJ3DataSubsystem::Get()->GetConfigFloat("weapon_default_cooldown_sec", 0.f);
	}
	if (CooldownSec <= 0.f)
	{
		return;
	}


	FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(Handle, ActorInfo, ActivationInfo, UJ3GE_WeaponCooldown::StaticClass(), GetAbilityLevel(Handle, ActorInfo));
	Spec.Data->SetSetByCallerMagnitude(J3Tags::Data_Cooldown, CooldownSec);
	ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, Spec);
}


void UJ3WeaponAbility::FireHitscan(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo& ActivationInfo) const
{
	// 피해는 서버만 확정한다
	const FJ3EquipmentDef* Weapon = GetWeaponDef(ActorInfo);
	AActor* Avatar = ActorInfo->AvatarActor.Get();
	if (!HasAuthority(&ActivationInfo) || Weapon == nullptr || Avatar == nullptr)
	{
		return;
	}


	// 눈 위치에서 시점 방향으로 사거리만큼
	FVector EyeLocation;
	FRotator EyeRotation;
	Avatar->GetActorEyesViewPoint(EyeLocation, EyeRotation);
	const FVector End = EyeLocation + EyeRotation.Vector() * Weapon->RangeCm;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(J3WeaponTrace), false, Avatar);
	FHitResult Hit;
	if (!Avatar->GetWorld()->LineTraceSingleByChannel(Hit, EyeLocation, End, ECC_Visibility, Params))
	{
		UE_LOG(LogJ3Combat, Display, TEXT("[%s] %s 공격: 맞은 것 없음 (사거리 %.0fcm)"), *GetNameSafe(Avatar), *Weapon->Id.ToString(), Weapon->RangeCm);
		return;
	}
	UE_LOG(LogJ3Combat, Display, TEXT("[%s] %s 공격: %s 적중 (%.0fcm)"), *GetNameSafe(Avatar), *Weapon->Id.ToString(), *GetNameSafe(Hit.GetActor()), Hit.Distance);


	ApplyDamage(ActorInfo, Hit.GetActor(), Weapon->Damage);
}


void UJ3WeaponAbility::ApplyDamage(const FGameplayAbilityActorInfo* ActorInfo, AActor* Target, float Damage) const
{
	// GAS가 있는 대상만 피해를 받는다 (몬스터·보스, 단계 7)
	// 팀원 공격(아군 피해)은 없다 (확정): 플레이어에게는 피해를 주지 않는다.
	UAbilitySystemComponent* TargetAbilitySystem = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	if (TargetAbilitySystem == nullptr || Damage <= 0.f || Cast<AJ3Character>(Target) != nullptr)
	{
		return;
	}
	UAbilitySystemComponent* SourceAbilitySystem = ActorInfo->AbilitySystemComponent.Get();
	FGameplayEffectSpecHandle Spec = SourceAbilitySystem->MakeOutgoingSpec(UJ3GE_Damage::StaticClass(), 1.f, SourceAbilitySystem->MakeEffectContext());
	Spec.Data->SetSetByCallerMagnitude(J3Tags::Data_Damage, -Damage);
	SourceAbilitySystem->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetAbilitySystem);
}


void UJ3WeaponAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	// 쿨다운·비용(탄약) 확인과 적용 후 판정
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	FireHitscan(ActorInfo, ActivationInfo);
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}


bool UJ3GA_RangedShot::CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	// 탄약이 1개 이상 있어야 쏜다 (인벤토리는 본인에게 복제되므로 클라이언트도 미리 판단할 수 있다)
	if (!Super::CheckCost(Handle, ActorInfo, OptionalRelevantTags))
	{
		return false;
	}
	const FJ3EquipmentDef* Weapon = GetWeaponDef(ActorInfo);
	const UJ3InventoryComponent* Inventory = J3WeaponAbilityPrivate::GetInventory(ActorInfo);
	if (Weapon == nullptr || Weapon->AmmoItem.IsNone())
	{
		return true;
	}
	return Inventory != nullptr && Inventory->CountItem(Weapon->AmmoItem) > 0;
}


void UJ3GA_RangedShot::ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	// 탄약 소모는 서버만 한다 (인벤토리는 서버 권한)
	Super::ApplyCost(Handle, ActorInfo, ActivationInfo);
	const FJ3EquipmentDef* Weapon = GetWeaponDef(ActorInfo);
	UJ3InventoryComponent* Inventory = J3WeaponAbilityPrivate::GetInventory(ActorInfo);
	if (HasAuthority(&ActivationInfo) && Weapon != nullptr && !Weapon->AmmoItem.IsNone() && Inventory != nullptr)
	{
		Inventory->RemoveItem(Weapon->AmmoItem, 1);
	}
}
