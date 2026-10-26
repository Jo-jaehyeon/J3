// Copyright (c) ACCENT. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayAbilitySpecHandle.h"
#include "J3EquipmentAbilityComponent.generated.h"


// 손에 든 장비에 맞춰 무기 능력(GA)을 교체한다 (서버 전용).
// 장비가 바뀌면 이전 GA를 해제하고, equipment.csv에 연결된 새 GA를 공격 입력 태그와 함께 부여한다.
UCLASS(ClassGroup = (J3))
class J3_API UJ3EquipmentAbilityComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UJ3EquipmentAbilityComponent();

protected:
	virtual void BeginPlay() override;

private:
	void OnHeldItemChanged(FName HeldItemId);
	void ClearWeaponAbility();

	FGameplayAbilitySpecHandle WeaponAbilityHandle;
};
