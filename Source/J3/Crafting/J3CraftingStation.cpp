// Copyright (c) ACCENT. All Rights Reserved.

#include "Crafting/J3CraftingStation.h"

#include "AbilitySystemComponent.h"
#include "Abilities/J3GameplayTags.h"
#include "Components/StaticMeshComponent.h"
#include "Core/J3PlayerController.h"
#include "Crafting/J3CraftingStationComponent.h"
#include "Player/J3Character.h"


AJ3CraftingStation::AJ3CraftingStation()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	RootComponent = Mesh;
	Station = CreateDefaultSubobject<UJ3CraftingStationComponent>(TEXT("Station"));
}


bool AJ3CraftingStation::CanInteract(const AJ3Character* Interactor) const
{
	const UAbilitySystemComponent* AbilitySystem = Interactor != nullptr ? Interactor->GetAbilitySystemComponent() : nullptr;
	return AbilitySystem == nullptr || !AbilitySystem->HasMatchingGameplayTag(J3Tags::State_Dead);
}


void AJ3CraftingStation::Interact(AJ3Character* Interactor)
{
	// 화면은 그 플레이어의 PC에서만 연다 (제작 자체는 화면에서 다시 서버에 요청한다)
	if (AJ3PlayerController* Controller = Cast<AJ3PlayerController>(Interactor->GetController()))
	{
		Controller->Client_OpenCraftingScreen(this);
	}
}
