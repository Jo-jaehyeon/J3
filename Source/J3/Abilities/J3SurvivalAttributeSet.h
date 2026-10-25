// Copyright (c) ACCENT. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "J3SurvivalAttributeSet.generated.h"

#define J3_ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)


// 플레이어 생존 수치: HP·공복·갈증과 각 최대치 (Player_And_Survival 2절).
// 값은 서버만 바꾸고, 모든 클라이언트가 HUD·상태 표시에 쓰도록 복제한다.
UCLASS()
class J3_API UJ3SurvivalAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	J3_ATTRIBUTE_ACCESSORS(UJ3SurvivalAttributeSet, Health);
	J3_ATTRIBUTE_ACCESSORS(UJ3SurvivalAttributeSet, MaxHealth);
	J3_ATTRIBUTE_ACCESSORS(UJ3SurvivalAttributeSet, Hunger);
	J3_ATTRIBUTE_ACCESSORS(UJ3SurvivalAttributeSet, MaxHunger);
	J3_ATTRIBUTE_ACCESSORS(UJ3SurvivalAttributeSet, Thirst);
	J3_ATTRIBUTE_ACCESSORS(UJ3SurvivalAttributeSet, MaxThirst);

protected:
	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_Hunger(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MaxHunger(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_Thirst(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MaxThirst(const FGameplayAttributeData& OldValue);

private:
	// 현재 값이 0~최대치를 벗어나지 않게 한다
	void ClampToMax(const FGameplayAttribute& Attribute, float& NewValue) const;


	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Health, Category = "J3|Survival", meta = (AllowPrivateAccess = true))
	FGameplayAttributeData Health;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHealth, Category = "J3|Survival", meta = (AllowPrivateAccess = true))
	FGameplayAttributeData MaxHealth;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Hunger, Category = "J3|Survival", meta = (AllowPrivateAccess = true))
	FGameplayAttributeData Hunger;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHunger, Category = "J3|Survival", meta = (AllowPrivateAccess = true))
	FGameplayAttributeData MaxHunger;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Thirst, Category = "J3|Survival", meta = (AllowPrivateAccess = true))
	FGameplayAttributeData Thirst;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxThirst, Category = "J3|Survival", meta = (AllowPrivateAccess = true))
	FGameplayAttributeData MaxThirst;
};
