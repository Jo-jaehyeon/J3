// Copyright (c) ACCENT. All Rights Reserved.

#include "Building/J3StructureBuildings.h"

#include "Crafting/J3CraftingStationComponent.h"


AJ3BaseCamp::AJ3BaseCamp()
{
	Workbench = CreateDefaultSubobject<UJ3CraftingStationComponent>(TEXT("Workbench"));
}


bool AJ3BaseCamp::CanInteract(const AJ3Character* Interactor) const
{
	return Workbench->CanOpenFor(Interactor);
}


void AJ3BaseCamp::Interact(AJ3Character* Interactor)
{
	Workbench->OpenFor(Interactor);
}
