// Copyright (c) ACCENT. All Rights Reserved.

#include "Core/J3PlayerController.h"

#include "EnhancedInputSubsystems.h"
#include "Building/J3BuilderComponent.h"
#include "Building/J3BuildingBase.h"
#include "Core/J3PlayerState.h"
#include "Data/J3DataSubsystem.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Inventory/J3InventoryComponent.h"
#include "Player/J3Character.h"
#include "UI/J3CraftingWidget.h"
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


	// 열려 있으면 닫고, 닫혀 있으면 연다
	if (InventoryWidget->IsInViewport())
	{
		InventoryWidget->RemoveFromParent();
		RefreshUIInputMode(nullptr);
		return;
	}
	InventoryWidget->AddToViewport(1);
	RefreshUIInputMode(InventoryWidget);
}


void AJ3PlayerController::Client_OpenCraftingScreen_Implementation(AActor* Station)
{
	// 처음 열 때 만든다
	if (CraftingWidget == nullptr)
	{
		const TSubclassOf<UJ3CraftingWidget> CraftingClass = CraftingWidgetClass != nullptr ? CraftingWidgetClass : TSubclassOf<UJ3CraftingWidget>(UJ3CraftingWidget::StaticClass());
		CraftingWidget = CreateWidget<UJ3CraftingWidget>(this, CraftingClass);
	}
	CraftingWidget->SetStation(Station);
	if (!CraftingWidget->IsInViewport())
	{
		CraftingWidget->AddToViewport(1);
	}
	RefreshUIInputMode(CraftingWidget);
}


void AJ3PlayerController::CloseCraftingScreen()
{
	if (CraftingWidget != nullptr && CraftingWidget->IsInViewport())
	{
		CraftingWidget->RemoveFromParent();
		RefreshUIInputMode(nullptr);
	}
}


void AJ3PlayerController::RefreshUIInputMode(UUserWidget* FocusWidget)
{
	// 모두 닫혔으면 게임 입력으로 돌아간다
	const bool bInventoryOpen = InventoryWidget != nullptr && InventoryWidget->IsInViewport();
	const bool bCraftingOpen = CraftingWidget != nullptr && CraftingWidget->IsInViewport();
	if (!bInventoryOpen && !bCraftingOpen)
	{
		SetInputMode(FInputModeGameOnly());
		SetShowMouseCursor(false);
		ResetIgnoreLookInput();
		return;
	}


	// 열려 있으면 커서로 다루고, 키 입력(I, 이동)은 게임에도 계속 들어간다 (인벤토리를 연 채 이동 가능, 확정)
	FInputModeGameAndUI InputMode;
	UUserWidget* Focus = FocusWidget != nullptr ? FocusWidget : (bCraftingOpen ? static_cast<UUserWidget*>(CraftingWidget) : static_cast<UUserWidget*>(InventoryWidget));
	InputMode.SetWidgetToFocus(Focus->TakeWidget());
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
	SetShowMouseCursor(true);
	if (!IsLookInputIgnored())
	{
		SetIgnoreLookInput(true);
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


bool AJ3PlayerController::TraceFromView(FHitResult& OutHit) const
{
	FVector ViewLocation;
	FRotator ViewRotation;
	GetPlayerViewPoint(ViewLocation, ViewRotation);
	const float RangeCm = UJ3DataSubsystem::Get()->GetConfigFloat("build_range_cm", 1500.f);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(J3BuildAim), true, GetPawn());
	return GetWorld()->LineTraceSingleByChannel(OutHit, ViewLocation, ViewLocation + ViewRotation.Vector() * RangeCm, ECC_Visibility, Params);
}


void AJ3PlayerController::J3Build(FName BuildingId, float YawDeg)
{
#if !UE_BUILD_SHIPPING
	FHitResult Hit;
	const AJ3PlayerState* J3PlayerState = GetPlayerState<AJ3PlayerState>();
	if (J3PlayerState != nullptr && TraceFromView(Hit))
	{
		J3PlayerState->GetBuilder()->Server_PlaceBuilding(BuildingId, Hit.ImpactPoint, YawDeg);
	}
#endif
}


void AJ3PlayerController::J3Demolish()
{
#if !UE_BUILD_SHIPPING
	FHitResult Hit;
	const AJ3PlayerState* J3PlayerState = GetPlayerState<AJ3PlayerState>();
	if (J3PlayerState != nullptr && TraceFromView(Hit))
	{
		J3PlayerState->GetBuilder()->Server_Demolish(Cast<AJ3BuildingBase>(Hit.GetActor()));
	}
#endif
}
