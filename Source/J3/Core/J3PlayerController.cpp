// Copyright (c) ACCENT. All Rights Reserved.

#include "Core/J3PlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Building/J3BuildModeComponent.h"
#include "Building/J3BuildVerify.h"
#include "Building/J3MachineBuildings.h"
#include "UI/J3MachineWidget.h"
#include "UI/J3StorageWidget.h"
#include "Building/J3MachineAccessComponent.h"
#include "Building/J3StorageBuilding.h"
#include "Building/J3BuilderComponent.h"
#include "Building/J3BuildingBase.h"
#include "Core/J3PlayerState.h"
#include "Data/J3DataSubsystem.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Inventory/J3InventoryComponent.h"
#include "Player/J3Character.h"
#include "UI/J3BuildMenuWidget.h"
#include "UI/J3CraftingWidget.h"
#include "UI/J3HUDWidget.h"
#include "UI/J3InventoryWidget.h"


AJ3PlayerController::AJ3PlayerController()
{
	BuildMode = CreateDefaultSubobject<UJ3BuildModeComponent>(TEXT("BuildMode"));
}


void AJ3PlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent);
	if (EnhancedInput == nullptr)
	{
		return;
	}
	if (BuildMenuAction != nullptr)
	{
		EnhancedInput->BindAction(BuildMenuAction, ETriggerEvent::Started, this, &AJ3PlayerController::ToggleBuildMenu);
	}
	if (ConfirmAction != nullptr)
	{
		EnhancedInput->BindAction(ConfirmAction, ETriggerEvent::Started, this, &AJ3PlayerController::OnConfirm);
		EnhancedInput->BindAction(ConfirmAction, ETriggerEvent::Completed, this, &AJ3PlayerController::OnConfirmReleased);
	}
	if (CancelAction != nullptr)
	{
		EnhancedInput->BindAction(CancelAction, ETriggerEvent::Started, this, &AJ3PlayerController::OnCancel);
	}
	if (PickBuildingAction != nullptr)
	{
		EnhancedInput->BindAction(PickBuildingAction, ETriggerEvent::Started, this, &AJ3PlayerController::OnPickBuilding);
	}
	if (DemolishModeAction != nullptr)
	{
		EnhancedInput->BindAction(DemolishModeAction, ETriggerEvent::Started, this, &AJ3PlayerController::OnDemolishMode);
	}
	if (ToggleGridAction != nullptr)
	{
		EnhancedInput->BindAction(ToggleGridAction, ETriggerEvent::Started, this, &AJ3PlayerController::OnToggleGrid);
	}
	if (HotbarAction != nullptr)
	{
		EnhancedInput->BindAction(HotbarAction, ETriggerEvent::Started, this, &AJ3PlayerController::OnHotbar);
	}
}


void AJ3PlayerController::OnConfirm()
{
	// 화면이 열려 있지 않을 때 좌클릭 = 설치 확정, 철거 모드면 누르고 있는 동안 철거 진행
	BuildMode->PressPrimary();
}


void AJ3PlayerController::OnConfirmReleased()
{
	BuildMode->ReleasePrimary();
}


void AJ3PlayerController::OnDemolishMode()
{
	BuildMode->ToggleDemolish();
}


void AJ3PlayerController::OnCancel()
{
	BuildMode->EndAll();
}


void AJ3PlayerController::OnPickBuilding()
{
	BuildMode->PickAimedBuilding();
}


void AJ3PlayerController::OnToggleGrid()
{
	BuildMode->ToggleGrid();
}


void AJ3PlayerController::OnHotbar(const FInputActionValue& Value)
{
	// 1~9, 0 → 칸 0~9
	const int32 Index = FMath::RoundToInt(Value.Get<float>()) - 1;
	BuildMode->UseHotbar(Index);
}


void AJ3PlayerController::ToggleBuildMenu()
{
	// 처음 열 때 만든다
	if (BuildMenuWidget == nullptr)
	{
		const TSubclassOf<UJ3BuildMenuWidget> MenuClass = BuildMenuWidgetClass != nullptr ? BuildMenuWidgetClass : TSubclassOf<UJ3BuildMenuWidget>(UJ3BuildMenuWidget::StaticClass());
		BuildMenuWidget = CreateWidget<UJ3BuildMenuWidget>(this, MenuClass);
	}
	if (BuildMenuWidget->IsInViewport())
	{
		CloseBuildMenu();
		return;
	}
	BuildMenuWidget->AddToViewport(1);
	RefreshUIInputMode(BuildMenuWidget);
}


void AJ3PlayerController::CloseBuildMenu()
{
	if (BuildMenuWidget != nullptr && BuildMenuWidget->IsInViewport())
	{
		BuildMenuWidget->RemoveFromParent();
		RefreshUIInputMode(nullptr);
	}
}


void AJ3PlayerController::BeginPlay()
{
	Super::BeginPlay();


	// 개발용 2인 건설 검증 (j3.Debug.AutoBuild, 평소에는 아무것도 하지 않는다)
	J3BuildVerify::StartAutoBuild(this);
	if (IsLocalController() && HasAuthority())
	{
		J3BuildVerify::StartAutoFactory(GetWorld());
	}


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


void AJ3PlayerController::Client_OpenMachineScreen_Implementation(AJ3MachineBuilding* Machine)
{
	// 처음 열 때 만든다
	if (MachineWidget == nullptr)
	{
		const TSubclassOf<UJ3MachineWidget> WidgetClass = MachineWidgetClass != nullptr ? MachineWidgetClass : TSubclassOf<UJ3MachineWidget>(UJ3MachineWidget::StaticClass());
		MachineWidget = CreateWidget<UJ3MachineWidget>(this, WidgetClass);
	}
	MachineWidget->SetMachine(Machine);
	if (!MachineWidget->IsInViewport())
	{
		MachineWidget->AddToViewport(1);
	}
	RefreshUIInputMode(MachineWidget);
}


void AJ3PlayerController::CloseMachineScreen()
{
	if (MachineWidget != nullptr && MachineWidget->IsInViewport())
	{
		MachineWidget->RemoveFromParent();
		RefreshUIInputMode(nullptr);
	}
}


void AJ3PlayerController::Client_OpenStorageScreen_Implementation(AJ3StorageBuilding* Storage, const TArray<FJ3ItemStack>& Slots)
{
	// 처음 열 때 만든다
	if (StorageWidget == nullptr)
	{
		const TSubclassOf<UJ3StorageWidget> WidgetClass = StorageWidgetClass != nullptr ? StorageWidgetClass : TSubclassOf<UJ3StorageWidget>(UJ3StorageWidget::StaticClass());
		StorageWidget = CreateWidget<UJ3StorageWidget>(this, WidgetClass);
	}
	StorageWidget->SetStorage(Storage, Slots);
	if (!StorageWidget->IsInViewport())
	{
		StorageWidget->AddToViewport(1);
	}
	RefreshUIInputMode(StorageWidget);
}


void AJ3PlayerController::Client_UpdateStorage_Implementation(AJ3StorageBuilding* Storage, const TArray<FJ3ItemStack>& Slots)
{
	if (StorageWidget != nullptr && StorageWidget->IsInViewport() && StorageWidget->GetStorage() == Storage)
	{
		StorageWidget->SetStorage(Storage, Slots);
	}
}


void AJ3PlayerController::CloseStorageScreen()
{
	// 서버에도 알려 더는 내용을 받지 않는다
	if (StorageWidget == nullptr || !StorageWidget->IsInViewport())
	{
		return;
	}
	const AJ3PlayerState* J3PlayerState = GetPlayerState<AJ3PlayerState>();
	if (J3PlayerState != nullptr && StorageWidget->GetStorage() != nullptr)
	{
		J3PlayerState->GetMachineAccess()->Server_CloseStorage(StorageWidget->GetStorage());
	}
	StorageWidget->RemoveFromParent();
	RefreshUIInputMode(nullptr);
}


void AJ3PlayerController::RefreshUIInputMode(UUserWidget* FocusWidget)
{
	// 모두 닫혔으면 게임 입력으로 돌아간다
	const bool bInventoryOpen = InventoryWidget != nullptr && InventoryWidget->IsInViewport();
	const bool bCraftingOpen = CraftingWidget != nullptr && CraftingWidget->IsInViewport();
	const bool bBuildMenuOpen = BuildMenuWidget != nullptr && BuildMenuWidget->IsInViewport();
	const bool bMachineOpen = MachineWidget != nullptr && MachineWidget->IsInViewport();
	const bool bStorageOpen = StorageWidget != nullptr && StorageWidget->IsInViewport();
	if (!bInventoryOpen && !bCraftingOpen && !bBuildMenuOpen && !bMachineOpen && !bStorageOpen)
	{
		SetInputMode(FInputModeGameOnly());
		SetShowMouseCursor(false);
		ResetIgnoreLookInput();
		return;
	}


	// 열려 있으면 커서로 다루고, 키 입력(I, 이동)은 게임에도 계속 들어간다 (인벤토리를 연 채 이동 가능, 확정)
	FInputModeGameAndUI InputMode;
	UUserWidget* Focus = FocusWidget;
	if (Focus == nullptr)
	{
		Focus = bBuildMenuOpen ? static_cast<UUserWidget*>(BuildMenuWidget) : (bCraftingOpen ? static_cast<UUserWidget*>(CraftingWidget) : (bMachineOpen ? static_cast<UUserWidget*>(MachineWidget) : (bStorageOpen ? static_cast<UUserWidget*>(StorageWidget) : static_cast<UUserWidget*>(InventoryWidget))));
	}
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
		J3PlayerState->GetBuilder()->Server_PlaceBuilding(BuildingId, Hit.ImpactPoint, YawDeg, false);
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
