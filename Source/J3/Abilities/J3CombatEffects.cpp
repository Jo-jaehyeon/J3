// Copyright (c) ACCENT. All Rights Reserved.

#include "Abilities/J3CombatEffects.h"

#include "Abilities/J3GameplayTags.h"
#include "Abilities/J3SurvivalAttributeSet.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"


UJ3GE_Damage::UJ3GE_Damage()
{
	FSetByCallerFloat SetByCaller;
	SetByCaller.DataTag = J3Tags::Data_Damage;

	FGameplayModifierInfo Modifier;
	Modifier.Attribute = UJ3SurvivalAttributeSet::GetHealthAttribute();
	Modifier.ModifierOp = EGameplayModOp::Additive;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);

	DurationPolicy = EGameplayEffectDurationType::Instant;
	Modifiers.Add(Modifier);
}


UJ3GE_WeaponCooldown::UJ3GE_WeaponCooldown()
{
	// 지속 시간
	FSetByCallerFloat SetByCaller;
	SetByCaller.DataTag = J3Tags::Data_Cooldown;
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);


	// 쿨다운 동안 붙는 태그 (GA가 이 태그로 쿨다운 여부를 판단한다)
	UTargetTagsGameplayEffectComponent* TargetTags = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("CooldownTags"));
	FInheritedTagContainer Tags;
	Tags.Added.AddTag(J3Tags::Cooldown_Weapon);
	TargetTags->SetAndApplyTargetTagChanges(Tags);
	GEComponents.Add(TargetTags);
}
