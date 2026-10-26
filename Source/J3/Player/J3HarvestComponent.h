// Copyright (c) ACCENT. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "J3HarvestComponent.generated.h"

class AJ3ResourceNode;


// 진행 중인 채취 (본인 클라이언트의 진행 막대 표시용)
USTRUCT()
struct FJ3HarvestProgress
{
	GENERATED_BODY()

	// 서버 시각 기준 시작 시각. 0이면 채취 중이 아니다.
	UPROPERTY()
	float StartServerTime = 0.f;

	UPROPERTY()
	float DurationSec = 0.f;
};


// 자원 노드 채취 (Player_And_Survival 1.1): G키로 시작하면 진행도가 차고, 다 차면 아이템을 얻는다.
// 맨손도 가능하고, 노드에 맞는 도구를 들면 빨라진다. 일부 노드(광맥)는 맞는 도구가 있어야 한다.
// 채취는 집중하는 행동이다: 채취 중에는 State.Harvesting 태그가 붙어 공격·장비 변경·점프·다른 상호작용을 막고,
// 1m 밖으로 멀어지거나 공격을 받으면 즉시 취소된다. 판정과 지급은 서버가 하고, 진행 상황은 본인에게만 복제한다.
UCLASS(ClassGroup = (J3))
class J3_API UJ3HarvestComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UJ3HarvestComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 지금 이 노드를 채취할 수 있는지 (서버·클라이언트 모두)
	bool CanHarvest(const AJ3ResourceNode& Node) const;

	// 서버: 채취 시작
	void StartHarvest(AJ3ResourceNode& Node);

	// 진행도 0~1 (채취 중이 아니면 음수)
	float GetProgress() const;

	// 서버: 진행 중인 채취를 취소한다
	void CancelHarvest();

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void FinishHarvest();
	void ClearHarvest();

	// 서버: 계속 채취할 수 있는 상태인지 (거리, 노드, 필요한 도구)
	bool IsHarvestStillValid() const;
	void CheckHarvest();
	void OnEffectAppliedToSelf(class UAbilitySystemComponent* Source, const struct FGameplayEffectSpec& Spec, struct FActiveGameplayEffectHandle Handle);
	class UAbilitySystemComponent* GetAbilitySystem() const;

	// 손에 든 장비의 equipment.csv 행 (맨손이거나 없으면 nullptr)
	const struct FJ3EquipmentDef* GetHeldEquipment() const;

	// 손에 든 도구가 이 노드에 주는 속도 배수 (맞지 않으면 1)
	float GetToolMultiplier(FName ToolTag) const;
	bool HasToolTag(FName Tag) const;

	UPROPERTY(Replicated)
	FJ3HarvestProgress Progress;

	TWeakObjectPtr<AJ3ResourceNode> HarvestingNode;
	FTimerHandle HarvestTimer;
	FTimerHandle CheckTimer;
	FDelegateHandle EffectAppliedHandle;
};
