// Copyright (c) ACCENT. All Rights Reserved.

#include "Crafting/J3CraftingStationComponent.h"

#include "AbilitySystemComponent.h"
#include "Abilities/J3GameplayTags.h"
#include "Core/J3PlayerController.h"
#include "Player/J3Character.h"


UJ3CraftingStationComponent::UJ3CraftingStationComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}


bool UJ3CraftingStationComponent::CanOpenFor(const AJ3Character* Interactor) const
{
	const UAbilitySystemComponent* AbilitySystem = Interactor != nullptr ? Interactor->GetAbilitySystemComponent() : nullptr;
	return Interactor != nullptr && (AbilitySystem == nullptr || !AbilitySystem->HasMatchingGameplayTag(J3Tags::State_Dead));
}


void UJ3CraftingStationComponent::OpenFor(AJ3Character* Interactor) const
{
	if (AJ3PlayerController* Controller = Cast<AJ3PlayerController>(Interactor->GetController()))
	{
		Controller->Client_OpenCraftingScreen(GetOwner());
	}
}
