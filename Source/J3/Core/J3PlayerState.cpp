// Copyright (c) ACCENT. All Rights Reserved.

#include "Core/J3PlayerState.h"

#include "AbilitySystemComponent.h"
#include "Abilities/J3EquipmentAbilityComponent.h"
#include "Abilities/J3SurvivalAttributeSet.h"
#include "Inventory/J3InventoryComponent.h"
#include "Player/J3SurvivalComponent.h"


AJ3PlayerState::AJ3PlayerState()
{
	// GAS: 효과 상세는 본인에게만, 태그·속성은 모두에게 (Mixed)
	AbilitySystem = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystem"));
	AbilitySystem->SetIsReplicated(true);
	AbilitySystem->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
	SurvivalAttributes = CreateDefaultSubobject<UJ3SurvivalAttributeSet>(TEXT("SurvivalAttributes"));


	Survival = CreateDefaultSubobject<UJ3SurvivalComponent>(TEXT("Survival"));
	Inventory = CreateDefaultSubobject<UJ3InventoryComponent>(TEXT("Inventory"));
	EquipmentAbilities = CreateDefaultSubobject<UJ3EquipmentAbilityComponent>(TEXT("EquipmentAbilities"));


	// PlayerState 기본 복제 빈도는 낮아서 HP 변화가 늦게 보이므로 올린다
	SetNetUpdateFrequency(100.f);
}


UAbilitySystemComponent* AJ3PlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystem;
}


void AJ3PlayerState::BeginPlay()
{
	Super::BeginPlay();

	// TODO(단계 8): 저장된 프로필이 있으면 그 값으로 복원한다
	if (HasAuthority())
	{
		Survival->InitializeAttributes(AbilitySystem);
	}
}
