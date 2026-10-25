// Copyright (c) ACCENT. All Rights Reserved.

#include "Abilities/J3SurvivalAttributeSet.h"

#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"


void UJ3SurvivalAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 팀원 상태 표시(쓰러짐 등)에 필요하므로 모든 클라이언트에 보낸다
	DOREPLIFETIME_CONDITION_NOTIFY(UJ3SurvivalAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UJ3SurvivalAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UJ3SurvivalAttributeSet, Hunger, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UJ3SurvivalAttributeSet, MaxHunger, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UJ3SurvivalAttributeSet, Thirst, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UJ3SurvivalAttributeSet, MaxThirst, COND_None, REPNOTIFY_Always);
}


void UJ3SurvivalAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	ClampToMax(Attribute, NewValue);
}


void UJ3SurvivalAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	// 즉시 효과는 기본값(Base)을 바꾼다. 현재값만 자르면 기본값이 최대치를 넘어 계속 쌓이므로 기본값을 자른다.
	const FGameplayAttribute& Attribute = Data.EvaluatedData.Attribute;
	const FGameplayAttributeData* AttributeData = Attribute.GetGameplayAttributeData(this);
	if (AttributeData == nullptr)
	{
		return;
	}
	const float OldBase = AttributeData->GetBaseValue();
	float NewBase = OldBase;
	ClampToMax(Attribute, NewBase);
	if (NewBase != OldBase)
	{
		GetOwningAbilitySystemComponentChecked()->SetNumericAttributeBase(Attribute, NewBase);
	}
}


void UJ3SurvivalAttributeSet::ClampToMax(const FGameplayAttribute& Attribute, float& NewValue) const
{
	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
	}
	else if (Attribute == GetHungerAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHunger());
	}
	else if (Attribute == GetThirstAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxThirst());
	}
}


void UJ3SurvivalAttributeSet::OnRep_Health(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UJ3SurvivalAttributeSet, Health, OldValue);
}


void UJ3SurvivalAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UJ3SurvivalAttributeSet, MaxHealth, OldValue);
}


void UJ3SurvivalAttributeSet::OnRep_Hunger(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UJ3SurvivalAttributeSet, Hunger, OldValue);
}


void UJ3SurvivalAttributeSet::OnRep_MaxHunger(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UJ3SurvivalAttributeSet, MaxHunger, OldValue);
}


void UJ3SurvivalAttributeSet::OnRep_Thirst(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UJ3SurvivalAttributeSet, Thirst, OldValue);
}


void UJ3SurvivalAttributeSet::OnRep_MaxThirst(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UJ3SurvivalAttributeSet, MaxThirst, OldValue);
}
