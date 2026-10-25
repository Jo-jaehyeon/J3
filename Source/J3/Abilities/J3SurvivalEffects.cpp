// Copyright (c) ACCENT. All Rights Reserved.

#include "Abilities/J3SurvivalEffects.h"

#include "Abilities/J3GameplayTags.h"
#include "Abilities/J3SurvivalAttributeSet.h"


namespace J3SurvivalEffectsPrivate
{
	FGameplayModifierInfo MakeSetByCallerAdd(const FGameplayAttribute& Attribute, const FGameplayTag& DataTag)
	{
		FSetByCallerFloat SetByCaller;
		SetByCaller.DataTag = DataTag;

		FGameplayModifierInfo Modifier;
		Modifier.Attribute = Attribute;
		Modifier.ModifierOp = EGameplayModOp::Additive;
		Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);
		return Modifier;
	}
}


UJ3GE_SurvivalTick::UJ3GE_SurvivalTick()
{
	using namespace J3SurvivalEffectsPrivate;

	DurationPolicy = EGameplayEffectDurationType::Instant;
	Modifiers.Add(MakeSetByCallerAdd(UJ3SurvivalAttributeSet::GetHealthAttribute(), J3Tags::Data_Survival_HealthDelta));
	Modifiers.Add(MakeSetByCallerAdd(UJ3SurvivalAttributeSet::GetHungerAttribute(), J3Tags::Data_Survival_HungerDelta));
	Modifiers.Add(MakeSetByCallerAdd(UJ3SurvivalAttributeSet::GetThirstAttribute(), J3Tags::Data_Survival_ThirstDelta));
}
