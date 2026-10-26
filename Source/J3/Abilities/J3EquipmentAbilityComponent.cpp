// Copyright (c) ACCENT. All Rights Reserved.

#include "Abilities/J3EquipmentAbilityComponent.h"

#include "AbilitySystemComponent.h"
#include "Abilities/J3GameplayTags.h"
#include "Abilities/J3WeaponAbility.h"
#include "Core/J3PlayerState.h"
#include "Data/J3DataSubsystem.h"
#include "Inventory/J3InventoryComponent.h"


UJ3EquipmentAbilityComponent::UJ3EquipmentAbilityComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}


void UJ3EquipmentAbilityComponent::BeginPlay()
{
	Super::BeginPlay();

	// 능력 부여는 서버만 한다 (GAS가 본인 클라이언트에 복제)
	AJ3PlayerState* PlayerState = Cast<AJ3PlayerState>(GetOwner());
	if (PlayerState == nullptr || !PlayerState->HasAuthority())
	{
		return;
	}
	UJ3InventoryComponent* Inventory = PlayerState->GetInventory();
	Inventory->OnHeldItemChanged.AddUObject(this, &UJ3EquipmentAbilityComponent::OnHeldItemChanged);
	OnHeldItemChanged(Inventory->GetHeldItemId());
}


void UJ3EquipmentAbilityComponent::OnHeldItemChanged(FName HeldItemId)
{
	// 이전 무기 능력 해제
	ClearWeaponAbility();


	// 새 장비의 능력 클래스 (맨손, equipment.csv에 없거나 공격이 없는 도구면 능력 없음)
	const FJ3EquipmentDef* Equipment = UJ3DataSubsystem::Get()->GetEquipment(HeldItemId);
	if (Equipment == nullptr || Equipment->Ability.IsNull())
	{
		return;
	}
	UClass* AbilityClass = Equipment->Ability.TryLoadClass<UJ3WeaponAbility>();
	if (AbilityClass == nullptr || !AbilityClass->IsChildOf(UJ3WeaponAbility::StaticClass()))
	{
		UE_LOG(LogJ3Data, Error, TEXT("equipment.csv '%s': ability '%s'이(가) UJ3WeaponAbility 하위 클래스가 아닙니다"), *HeldItemId.ToString(), *Equipment->Ability.ToString());
		return;
	}


	// 공격 버튼 태그와 함께 부여
	UAbilitySystemComponent* AbilitySystem = Cast<AJ3PlayerState>(GetOwner())->GetAbilitySystemComponent();
	FGameplayAbilitySpec Spec(AbilityClass, 1);
	Spec.GetDynamicSpecSourceTags().AddTag(J3Tags::InputTag_Attack);
	WeaponAbilityHandle = AbilitySystem->GiveAbility(Spec);
}


void UJ3EquipmentAbilityComponent::ClearWeaponAbility()
{
	if (!WeaponAbilityHandle.IsValid())
	{
		return;
	}
	AJ3PlayerState* PlayerState = Cast<AJ3PlayerState>(GetOwner());
	PlayerState->GetAbilitySystemComponent()->ClearAbility(WeaponAbilityHandle);
	WeaponAbilityHandle = FGameplayAbilitySpecHandle();
}
