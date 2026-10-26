// Copyright (c) ACCENT. All Rights Reserved.

#include "Core/J3PlayerController.h"

#include "EnhancedInputSubsystems.h"
#include "Core/J3PlayerState.h"
#include "Data/J3DataSubsystem.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Inventory/J3InventoryComponent.h"
#include "Player/J3Character.h"
#include "UI/J3HUDWidget.h"
#include "UI/J3InventoryWidget.h"


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


	// HUD
	const TSubclassOf<UJ3HUDWidget> HUDClass = HUDWidgetClass != nullptr ? HUDWidgetClass : TSubclassOf<UJ3HUDWidget>(UJ3HUDWidget::StaticClass());
	HUDWidget = CreateWidget<UJ3HUDWidget>(this, HUDClass);
	HUDWidget->AddToViewport();
}


void AJ3PlayerController::ToggleInventoryScreen()
{
	// 처음 열 때 만든다
	if (InventoryWidget == nullptr)
	{
		const TSubclassOf<UJ3InventoryWidget> InventoryClass = InventoryWidgetClass != nullptr ? InventoryWidgetClass : TSubclassOf<UJ3InventoryWidget>(UJ3InventoryWidget::StaticClass());
		InventoryWidget = CreateWidget<UJ3InventoryWidget>(this, InventoryClass);
	}


	// 닫기: 게임 입력으로 돌아간다
	if (InventoryWidget->IsInViewport())
	{
		InventoryWidget->RemoveFromParent();
		SetInputMode(FInputModeGameOnly());
		SetShowMouseCursor(false);
		ResetIgnoreLookInput();
		return;
	}


	// 열기: 커서로 칸을 다루고, 키 입력(I, 이동)은 게임에도 계속 들어간다
	// 인벤토리를 연 채로 이동할 수 있다 (확정)
	InventoryWidget->AddToViewport(1);
	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(InventoryWidget->TakeWidget());
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
	SetShowMouseCursor(true);
	SetIgnoreLookInput(true);
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


void AJ3PlayerController::UpdateRotation(float DeltaTime)
{
	Super::UpdateRotation(DeltaTime);

	// 채취를 시작한 순간의 방향을 기준으로 삼는다
	const AJ3Character* J3Character = Cast<AJ3Character>(GetPawn());
	const bool bHarvesting = J3Character != nullptr && J3Character->IsHarvesting();
	if (bHarvesting && !bWasHarvesting)
	{
		HarvestLookCenter = GetControlRotation();
	}
	bWasHarvesting = bHarvesting;
	if (!bHarvesting)
	{
		return;
	}


	// 기준에서 좌우·위아래로 설정 각도까지만 돌릴 수 있다 (0이면 고정)
	const UJ3DataSubsystem* Data = UJ3DataSubsystem::Get();
	const float YawLimit = FMath::Max(0.f, Data->GetConfigFloat("harvest_look_yaw_deg", 30.f));
	const float PitchLimit = FMath::Max(0.f, Data->GetConfigFloat("harvest_look_pitch_deg", 20.f));
	const FRotator Delta = (GetControlRotation() - HarvestLookCenter).GetNormalized();
	FRotator Clamped = HarvestLookCenter;
	Clamped.Yaw += FMath::Clamp(Delta.Yaw, -YawLimit, YawLimit);
	Clamped.Pitch += FMath::Clamp(Delta.Pitch, -PitchLimit, PitchLimit);
	SetControlRotation(Clamped.GetNormalized());
}
