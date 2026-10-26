// Copyright (c) ACCENT. All Rights Reserved.

#include "Core/J3GameState.h"

#include "Data/J3DataTypes.h"
#include "Net/UnrealNetwork.h"


void AJ3GameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AJ3GameState, WorldSeed);
	DOREPLIFETIME(AJ3GameState, ObtainedItems);
}


void AJ3GameState::NotifyItemObtained(FName ItemId)
{
	check(HasAuthority());
	ObtainedItems.AddUnique(ItemId);
}


bool AJ3GameState::IsUnlocked(const FJ3UnlockRule& Rule) const
{
	// on_obtain은 조건 아이템 중 하나라도 얻었으면 해방
	if (Rule.Type == EJ3UnlockType::Auto)
	{
		return true;
	}
	for (const FName& ItemId : Rule.Items)
	{
		if (ObtainedItems.Contains(ItemId))
		{
			return true;
		}
	}
	return false;
}
