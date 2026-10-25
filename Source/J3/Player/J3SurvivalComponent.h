// Copyright (c) ACCENT. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "J3SurvivalComponent.generated.h"

class UAbilitySystemComponent;


// 공복·갈증 감소, 굶주림 HP 감소, 자연 회복, 사망 판정 (Player_And_Survival 2절).
// 서버에서만 일정 간격으로 계산해 GAS 효과(UJ3GE_SurvivalTick)로 반영한다. 수치는 모두 config.csv.
// 플레이어 개인 데이터라 폰이 바뀌어도 유지되도록 PlayerState에 붙인다.
UCLASS(ClassGroup = (J3), meta = (BlueprintSpawnableComponent))
class J3_API UJ3SurvivalComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UJ3SurvivalComponent();

	// 서버: 최대치와 현재 값을 설정 값으로 채운다 (새 프로필)
	void InitializeAttributes(UAbilitySystemComponent* AbilitySystem);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void TickSurvival();
	void UpdateStateTags(UAbilitySystemComponent* AbilitySystem) const;

	UAbilitySystemComponent* GetAbilitySystem() const;

	FTimerHandle TickTimer;

	float TickIntervalSec = 1.f;
};
