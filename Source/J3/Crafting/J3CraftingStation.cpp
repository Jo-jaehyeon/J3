// Copyright (c) ACCENT. All Rights Reserved.

#include "Crafting/J3CraftingStation.h"

#include "Components/StaticMeshComponent.h"
#include "Crafting/J3CraftingStationComponent.h"


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
	return Station->CanOpenFor(Interactor);
}


void AJ3CraftingStation::Interact(AJ3Character* Interactor)
{
	Station->OpenFor(Interactor);
}
