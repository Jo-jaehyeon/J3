// Copyright (c) ACCENT. All Rights Reserved.

#include "Core/J3PlayerController.h"

#include "EnhancedInputSubsystems.h"
#include "Core/J3PlayerState.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Inventory/J3InventoryComponent.h"


void AJ3PlayerController::BeginPlay()
{
	Super::BeginPlay();


	// 입력은 이 PC에서 조작하는 컨트롤러에만 등록한다 (서버에 있는 다른 플레이어의 컨트롤러 제외)
	if (!IsLocalController() || DefaultMappingContext == nullptr)
	{
		return;
	}
	UEnhancedInputLocalPlayerSubsystem* InputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	if (InputSubsystem != nullptr)
	{
		InputSubsystem->AddMappingContext(DefaultMappingContext, 0);
	}
}


void AJ3PlayerController::J3Give(FName ItemId, int32 Count)
{
#if !UE_BUILD_SHIPPING
	Server_DebugGive(ItemId, Count);
#endif
}


void AJ3PlayerController::Server_DebugGive_Implementation(FName ItemId, int32 Count)
{
#if !UE_BUILD_SHIPPING
	const AJ3PlayerState* J3PlayerState = GetPlayerState<AJ3PlayerState>();
	if (J3PlayerState != nullptr)
	{
		J3PlayerState->GetInventory()->AddItem(ItemId, Count);
	}
#endif
}


void AJ3PlayerController::J3Equip(int32 SlotIndex)
{
#if !UE_BUILD_SHIPPING
	const AJ3PlayerState* J3PlayerState = GetPlayerState<AJ3PlayerState>();
	if (J3PlayerState != nullptr)
	{
		J3PlayerState->GetInventory()->Server_EquipFromSlot(SlotIndex);
	}
#endif
}
