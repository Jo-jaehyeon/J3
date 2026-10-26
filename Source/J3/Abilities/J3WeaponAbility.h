// Copyright (c) ACCENT. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "J3WeaponAbility.generated.h"

struct FJ3EquipmentDef;


// 무기 공격 능력의 기본 클래스. 손에 든 장비가 바뀔 때마다 그 장비의 GA가 부여되고(UJ3EquipmentAbilityComponent),
// 공격 버튼(우클릭)은 무기와 상관없이 InputTag.Attack이 붙은 능력을 발동한다.
// 수치(피해·사거리·쿨다운·탄약)는 equipment.csv의 손에 든 장비 행에서 읽는다. 연출은 블루프린트 하위 클래스가 맡는다.
UCLASS(Abstract)
class J3_API UJ3WeaponAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UJ3WeaponAbility();

	virtual const FGameplayTagContainer* GetCooldownTags() const override;
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;

protected:
	// 기본 동작: 쿨다운·비용 확인과 적용(Commit) → 시점 방향 판정. 연출은 블루프린트에서 덧붙인다.
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	// 손에 든 장비의 equipment.csv 행 (없으면 nullptr)
	const FJ3EquipmentDef* GetWeaponDef(const FGameplayAbilityActorInfo* ActorInfo) const;

	// 시점 방향으로 사거리만큼 판정하고, 맞은 대상에 피해를 준다 (서버에서만 피해 적용)
	void FireHitscan(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo& ActivationInfo) const;

	void ApplyDamage(const FGameplayAbilityActorInfo* ActorInfo, AActor* Target, float Damage) const;
};


// 근접 공격 (도끼). TODO(단계 7): 휘두르기 범위 판정
UCLASS()
class J3_API UJ3GA_MeleeSwing : public UJ3WeaponAbility
{
	GENERATED_BODY()
};


// 원거리 공격 (활): 쏠 때마다 탄약 1개 소모
// TODO(단계 7): 즉시 판정 대신 화살 투사체
UCLASS()
class J3_API UJ3GA_RangedShot : public UJ3WeaponAbility
{
	GENERATED_BODY()

public:
	virtual bool CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;
};
