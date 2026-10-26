// Copyright (c) ACCENT. All Rights Reserved.

#include "Core/J3GameState.h"

#include "Net/UnrealNetwork.h"


void AJ3GameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AJ3GameState, WorldSeed);
}
