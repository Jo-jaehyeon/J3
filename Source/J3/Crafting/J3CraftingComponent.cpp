// Copyright (c) ACCENT. All Rights Reserved.

#include "Crafting/J3CraftingComponent.h"

#include "Core/J3GameState.h"
#include "Core/J3PlayerState.h"
#include "Crafting/J3CraftingStationComponent.h"
#include "Data/J3DataSubsystem.h"
#include "Engine/World.h"
#include "Inventory/J3InventoryComponent.h"
#include "Player/J3Character.h"


UJ3CraftingComponent::UJ3CraftingComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}


bool UJ3CraftingComponent::IsRecipeAvailableAt(const FJ3RecipeDef& Recipe, const AActor* Station) const
{
	// 장소가 맞고 팀이 해방한 레시피
	const UJ3CraftingStationComponent* StationComponent = Station != nullptr ? Station->FindComponentByClass<UJ3CraftingStationComponent>() : nullptr;
	const AJ3GameState* GameState = GetWorld()->GetGameState<AJ3GameState>();
	return StationComponent != nullptr && GameState != nullptr && Recipe.Stations.Contains(StationComponent->GetStationId()) && GameState->IsUnlocked(Recipe.Unlock);
}


bool UJ3CraftingComponent::IsNearStation(const AActor* Station) const
{
	const AJ3PlayerState* PlayerState = Cast<AJ3PlayerState>(GetOwner());
	const AJ3Character* Character = PlayerState != nullptr ? PlayerState->GetPawn<AJ3Character>() : nullptr;
	if (Character == nullptr || Station == nullptr)
	{
		return false;
	}
	const UJ3DataSubsystem* Data = UJ3DataSubsystem::Get();
	const float AllowedCm = Data->GetConfigFloat("interact_range_cm", 100.f) + Data->GetConfigFloat("interact_tolerance_cm", 0.f);
	return Character->GetBodyDistanceTo(Station) <= AllowedCm;
}


void UJ3CraftingComponent::Server_Craft_Implementation(AActor* Station, FName RecipeId)
{
	// 장소·해방·거리 확인
	const FJ3RecipeDef* Recipe = UJ3DataSubsystem::Get()->GetRecipe(RecipeId);
	if (Recipe == nullptr || !IsRecipeAvailableAt(*Recipe, Station) || !IsNearStation(Station))
	{
		return;
	}


	// 재료 → 결과물 (모자라거나 자리가 없으면 아무것도 바뀌지 않는다)
	// TODO(단계 1 UI): 실패 사유 안내 (재료 부족, 가방 가득 참)
	Cast<AJ3PlayerState>(GetOwner())->GetInventory()->TryExchange(Recipe->Inputs, Recipe->Output);
}
