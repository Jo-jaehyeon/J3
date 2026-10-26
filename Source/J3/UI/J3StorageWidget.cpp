// Copyright (c) ACCENT. All Rights Reserved.

#include "UI/J3StorageWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Building/J3MachineAccessComponent.h"
#include "Building/J3StorageBuilding.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Core/J3PlayerController.h"
#include "Core/J3PlayerState.h"
#include "Data/J3DataSubsystem.h"
#include "Inventory/J3InventoryComponent.h"

#define LOCTEXT_NAMESPACE "J3Storage"


void UJ3StorageWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// 배경을 어둡게 덮고 가운데에 [제목, 창고 칸, 가방]
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
	WidgetTree->RootWidget = Root;
	UBorder* Dim = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Dim->SetBrushColor(DimColor);
	UCanvasPanelSlot* DimSlot = Root->AddChildToCanvas(Dim);
	DimSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	DimSlot->SetOffsets(FMargin(0.f));


	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	UCanvasPanelSlot* ColumnSlot = Root->AddChildToCanvas(Column);
	ColumnSlot->SetAnchors(FAnchors(0.5f, 0.5f));
	ColumnSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	ColumnSlot->SetAutoSize(true);
	TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Column->AddChildToVerticalBox(TitleText)->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
	StorageGrid = WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass());
	StorageGrid->SetSlotPadding(FMargin(2.f));
	Column->AddChildToVerticalBox(StorageGrid)->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));
	UTextBlock* BagLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	BagLabel->SetText(LOCTEXT("Bag", "가방"));
	Column->AddChildToVerticalBox(BagLabel);
	BagGrid = WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass());
	BagGrid->SetSlotPadding(FMargin(2.f));
	Column->AddChildToVerticalBox(BagGrid);
	UTextBlock* Hint = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Hint->SetText(LOCTEXT("Hint", "가방 칸 클릭: 넣기   창고 칸 클릭: 꺼내기   끌기: 옮기기   Esc: 닫기"));
	Column->AddChildToVerticalBox(Hint)->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));
	SetIsFocusable(true);
}


UJ3ItemSlotWidget* UJ3StorageWidget::MakeSlot(EJ3SlotKind Kind, int32 Index)
{
	const TSubclassOf<UJ3ItemSlotWidget> SlotClass = SlotWidgetClass != nullptr ? SlotWidgetClass : TSubclassOf<UJ3ItemSlotWidget>(UJ3ItemSlotWidget::StaticClass());
	UJ3ItemSlotWidget* SlotWidget = CreateWidget<UJ3ItemSlotWidget>(GetOwningPlayer(), SlotClass);
	SlotWidget->Setup(Kind, Index, this, FText::GetEmpty());
	return SlotWidget;
}


void UJ3StorageWidget::SetStorage(AJ3StorageBuilding* InStorage, const TArray<FJ3ItemStack>& InSlots)
{
	Storage = InStorage;
	StorageItems = InSlots;
	const FJ3BuildingDef* Def = InStorage != nullptr ? InStorage->GetDef() : nullptr;
	TitleText->SetText(Def != nullptr ? Def->DisplayName : FText::GetEmpty());
	RefreshGrid(StorageGrid, StorageSlots, EJ3SlotKind::Storage, StorageItems);
}


void UJ3StorageWidget::RefreshGrid(UUniformGridPanel* Grid, TArray<TObjectPtr<UJ3ItemSlotWidget>>& Widgets, EJ3SlotKind Kind, const TArray<FJ3ItemStack>& Items)
{
	// 칸 수가 바뀌면 다시 만든다
	const int32 Columns = FMath::Max(1, UJ3DataSubsystem::Get()->GetConfigInt("inventory_columns", 10));
	if (Widgets.Num() != Items.Num())
	{
		Grid->ClearChildren();
		Widgets.Reset();
		for (int32 Index = 0; Index < Items.Num(); ++Index)
		{
			UJ3ItemSlotWidget* SlotWidget = MakeSlot(Kind, Index);
			Grid->AddChildToUniformGrid(SlotWidget, Index / Columns, Index % Columns);
			Widgets.Add(SlotWidget);
		}
	}
	for (int32 Index = 0; Index < Items.Num(); ++Index)
	{
		Widgets[Index]->SetItem(Items[Index]);
	}
}


void UJ3StorageWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 창고가 사라지거나 멀어지면 닫는다
	const AJ3PlayerState* PlayerState = GetOwningPlayerState<AJ3PlayerState>();
	AJ3PlayerController* Controller = Cast<AJ3PlayerController>(GetOwningPlayer());
	if (PlayerState != nullptr && Controller != nullptr && !PlayerState->GetMachineAccess()->IsNearBuilding(Storage.Get()))
	{
		Controller->CloseStorageScreen();
		return;
	}
	if (PlayerState != nullptr)
	{
		RefreshGrid(BagGrid, BagSlots, EJ3SlotKind::Inventory, PlayerState->GetInventory()->GetSlots());
	}
}


FReply UJ3StorageWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		if (AJ3PlayerController* Controller = Cast<AJ3PlayerController>(GetOwningPlayer()))
		{
			Controller->CloseStorageScreen();
		}
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}


void UJ3StorageWidget::HandleSlotClicked(EJ3SlotKind Kind, int32 Index)
{
	// 가방 → 창고, 창고 → 가방 (판정은 서버)
	const AJ3PlayerState* PlayerState = GetOwningPlayerState<AJ3PlayerState>();
	if (PlayerState == nullptr || !Storage.IsValid())
	{
		return;
	}
	if (Kind == EJ3SlotKind::Inventory)
	{
		PlayerState->GetMachineAccess()->Server_StoreFromInventory(Storage.Get(), Index);
	}
	else if (Kind == EJ3SlotKind::Storage)
	{
		PlayerState->GetMachineAccess()->Server_TakeFromStorage(Storage.Get(), Index);
	}
}


void UJ3StorageWidget::HandleSlotDropped(EJ3SlotKind FromKind, int32 FromIndex, EJ3SlotKind ToKind, int32 ToIndex)
{
	// 같은 쪽 안에서는 옮기기, 다른 쪽으로는 클릭과 같다
	const AJ3PlayerState* PlayerState = GetOwningPlayerState<AJ3PlayerState>();
	if (PlayerState == nullptr || !Storage.IsValid())
	{
		return;
	}
	if (FromKind == EJ3SlotKind::Inventory && ToKind == EJ3SlotKind::Inventory)
	{
		PlayerState->GetInventory()->Server_MoveSlot(FromIndex, ToIndex);
	}
	else if (FromKind == EJ3SlotKind::Storage && ToKind == EJ3SlotKind::Storage)
	{
		PlayerState->GetMachineAccess()->Server_MoveStorageSlot(Storage.Get(), FromIndex, ToIndex);
	}
	else
	{
		HandleSlotClicked(FromKind, FromIndex);
	}
}

#undef LOCTEXT_NAMESPACE
