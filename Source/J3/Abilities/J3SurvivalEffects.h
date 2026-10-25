// Copyright (c) ACCENT. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "J3SurvivalEffects.generated.h"


// 생존 틱 한 번의 변화량 (HP·공복·갈증). 크기는 SetByCaller로 넣으므로 이 클래스에는 수치가 없다.
// 수치는 config.csv → UJ3SurvivalComponent가 계산한다.
UCLASS()
class J3_API UJ3GE_SurvivalTick : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UJ3GE_SurvivalTick();
};
