// Copyright (c) ACCENT. All Rights Reserved.

#include "Player/J3HarvestComponent.h"

#include "AbilitySystemComponent.h"
#include "Abilities/J3CombatEffects.h"
#include "Abilities/J3GameplayTags.h"
#include "Core/J3PlayerState.h"
#include "Data/J3DataSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "Interaction/J3ResourceNode.h"
#include "Inventory/J3InventoryComponent.h"
#include "Net/UnrealNetwork.h"
#include "Player/J3Character.h"
#include "TimerManager.h"


UJ3HarvestComponent::UJ3HarvestComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}


void UJ3HarvestComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(UJ3HarvestComponent, Progress, COND_OwnerOnly);
}


void UJ3HarvestComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearHarvest();
	Super::EndPlay(EndPlayReason);
}


const FJ3EquipmentDef* UJ3HarvestComponent::GetHeldEquipment() const
{
	const AJ3Character* Character = Cast<AJ3Character>(GetOwner());
	const AJ3PlayerState* PlayerState = Character != nullptr ? Character->GetPlayerState<AJ3PlayerState>() : nullptr;
	return PlayerState != nullptr ? UJ3DataSubsystem::Get()->GetEquipment(PlayerState->GetInventory()->GetHeldItemId()) : nullptr;
}


bool UJ3HarvestComponent::HasToolTag(FName Tag) const
{
	const FJ3EquipmentDef* Held = GetHeldEquipment();
	return Held != nullptr && !Tag.IsNone() && Held->HarvestTag == Tag;
}


float UJ3HarvestComponent::GetToolMultiplier(FName ToolTag) const
{
	return HasToolTag(ToolTag) ? FMath::Max(0.01f, GetHeldEquipment()->HarvestMultiplier) : 1.f;
}


bool UJ3HarvestComponent::CanHarvest(const AJ3ResourceNode& Node) const
{
	// 이미 채취 중이면 새로 시작하지 않는다
	const FJ3ResourceNodeDef* Def = Node.GetDef();
	if (Def == nullptr || Progress.StartServerTime > 0.f)
	{
		return false;
	}


	// 사망 중이 아니고, 필요한 도구가 있으면 들고 있어야 한다
	const AJ3Character* Character = Cast<AJ3Character>(GetOwner());
	const UAbilitySystemComponent* AbilitySystem = Character != nullptr ? Character->GetAbilitySystemComponent() : nullptr;
	if (AbilitySystem != nullptr && AbilitySystem->HasMatchingGameplayTag(J3Tags::State_Dead))
	{
		return false;
	}
	return Def->RequiredTag.IsNone() || HasToolTag(Def->RequiredTag);
}


UAbilitySystemComponent* UJ3HarvestComponent::GetAbilitySystem() const
{
	const AJ3Character* Character = Cast<AJ3Character>(GetOwner());
	return Character != nullptr ? Character->GetAbilitySystemComponent() : nullptr;
}


void UJ3HarvestComponent::StartHarvest(AJ3ResourceNode& Node)
{
	check(GetOwner()->HasAuthority());

	// 시간 = 맨손 기준 시간 / 도구 배수
	const FJ3ResourceNodeDef* Def = Node.GetDef();
	UAbilitySystemComponent* AbilitySystem = GetAbilitySystem();
	if (!CanHarvest(Node) || AbilitySystem == nullptr)
	{
		return;
	}
	const float DurationSec = Def->HarvestSec / GetToolMultiplier(Def->ToolTag);
	HarvestingNode = &Node;
	Progress.StartServerTime = GetWorld()->GetGameState()->GetServerWorldTimeSeconds();
	Progress.DurationSec = DurationSec;
	GetWorld()->GetTimerManager().SetTimer(HarvestTimer, this, &UJ3HarvestComponent::FinishHarvest, DurationSec, false);


	// 채취 중 상태: 다른 행동을 막는 태그 (모든 클라이언트가 알도록 복제)
	AbilitySystem->SetLooseGameplayTagCount(J3Tags::State_Harvesting, 1, EGameplayTagReplicationState::TagOnly);


	// 취소 조건 감시: 멀어짐(주기적으로 거리 확인), 공격받음(피해 효과 적용)
	const float CheckSec = FMath::Max(0.02f, UJ3DataSubsystem::Get()->GetConfigFloat("harvest_check_sec", 0.1f));
	GetWorld()->GetTimerManager().SetTimer(CheckTimer, this, &UJ3HarvestComponent::CheckHarvest, CheckSec, true);
	EffectAppliedHandle = AbilitySystem->OnGameplayEffectAppliedDelegateToSelf.AddUObject(this, &UJ3HarvestComponent::OnEffectAppliedToSelf);
}


bool UJ3HarvestComponent::IsHarvestStillValid() const
{
	// 노드가 있고, 살아 있고, 1m(+여유) 안이고, 필요한 도구를 아직 들고 있어야 한다
	const AJ3ResourceNode* Node = HarvestingNode.Get();
	const AJ3Character* Character = Cast<AJ3Character>(GetOwner());
	const FJ3ResourceNodeDef* Def = Node != nullptr ? Node->GetDef() : nullptr;
	const UAbilitySystemComponent* AbilitySystem = GetAbilitySystem();
	if (Def == nullptr || Character == nullptr || (AbilitySystem != nullptr && AbilitySystem->HasMatchingGameplayTag(J3Tags::State_Dead)))
	{
		return false;
	}
	const UJ3DataSubsystem* Data = UJ3DataSubsystem::Get();
	const float AllowedCm = Data->GetConfigFloat("interact_range_cm", 100.f) + Data->GetConfigFloat("interact_tolerance_cm", 0.f);
	return Character->GetBodyDistanceTo(Node) <= AllowedCm && (Def->RequiredTag.IsNone() || HasToolTag(Def->RequiredTag));
}


void UJ3HarvestComponent::CheckHarvest()
{
	if (!IsHarvestStillValid())
	{
		CancelHarvest();
	}
}


void UJ3HarvestComponent::OnEffectAppliedToSelf(UAbilitySystemComponent* Source, const FGameplayEffectSpec& Spec, FActiveGameplayEffectHandle Handle)
{
	// 공격(피해 효과)을 받으면 취소. 굶주림 HP 감소는 공격이 아니므로 제외한다.
	if (Spec.Def != nullptr && Spec.Def->IsA<UJ3GE_Damage>())
	{
		CancelHarvest();
	}
}


void UJ3HarvestComponent::CancelHarvest()
{
	ClearHarvest();
}


void UJ3HarvestComponent::FinishHarvest()
{
	// 마지막 순간에도 확인한다 (확인 주기 사이에 멀어졌을 수 있다)
	const bool bValid = IsHarvestStillValid();
	const AJ3ResourceNode* Node = HarvestingNode.Get();
	const FJ3ResourceNodeDef* Def = Node != nullptr ? Node->GetDef() : nullptr;
	const AJ3Character* Character = Cast<AJ3Character>(GetOwner());
	ClearHarvest();
	if (!bValid)
	{
		return;
	}


	// 지급 (가방이 차서 못 넣은 것은 버린다)
	// TODO(단계 1 UI): 가방이 가득 찼다는 안내
	const AJ3PlayerState* PlayerState = Character->GetPlayerState<AJ3PlayerState>();
	if (PlayerState != nullptr)
	{
		PlayerState->GetInventory()->AddItem(Def->ItemId, Def->Yield);
	}
}


void UJ3HarvestComponent::ClearHarvest()
{
	// 타이머·감시 해제
	if (GetWorld() != nullptr)
	{
		GetWorld()->GetTimerManager().ClearTimer(HarvestTimer);
		GetWorld()->GetTimerManager().ClearTimer(CheckTimer);
	}
	UAbilitySystemComponent* AbilitySystem = GetAbilitySystem();
	if (AbilitySystem != nullptr)
	{
		AbilitySystem->OnGameplayEffectAppliedDelegateToSelf.Remove(EffectAppliedHandle);
		if (GetOwner()->HasAuthority())
		{
			AbilitySystem->SetLooseGameplayTagCount(J3Tags::State_Harvesting, 0, EGameplayTagReplicationState::TagOnly);
		}
	}
	EffectAppliedHandle.Reset();


	HarvestingNode.Reset();
	Progress = FJ3HarvestProgress();
}


float UJ3HarvestComponent::GetProgress() const
{
	const AGameStateBase* GameState = GetWorld() != nullptr ? GetWorld()->GetGameState() : nullptr;
	if (Progress.StartServerTime <= 0.f || Progress.DurationSec <= 0.f || GameState == nullptr)
	{
		return -1.f;
	}
	return FMath::Clamp((GameState->GetServerWorldTimeSeconds() - Progress.StartServerTime) / Progress.DurationSec, 0.f, 1.f);
}
