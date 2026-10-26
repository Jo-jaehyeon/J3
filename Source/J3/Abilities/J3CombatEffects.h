// Copyright (c) ACCENT. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "J3CombatEffects.generated.h"


// 즉시 피해. HP 감소량은 SetByCaller(Data.Damage, 음수)로 넣는다. 수치는 equipment.csv.
UCLASS()
class J3_API UJ3GE_Damage : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UJ3GE_Damage();
};


// 무기 쿨다운. 지속 시간은 SetByCaller(Data.Cooldown)이고, 그동안 Cooldown.Weapon 태그를 붙인다.
UCLASS()
class J3_API UJ3GE_WeaponCooldown : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UJ3GE_WeaponCooldown();
};
