// Copyright (c) ACCENT. All Rights Reserved.

#include "UI/J3InventoryWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Core/J3PlayerState.h"
#include "Data/J3DataSubsystem.h"
#include "Inventory/J3InventoryComponent.h"

#define LOCTEXT_NAMESPACE "J3Inventory"


namespace J3InventoryWidgetPrivate
{
	UVerticalBox* AddTitledColumn(UWidgetTree* Tree, UHorizontalBox* Parent, const FText& Title)
	{
		UVerticalBox* Column = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		UTextBlock* TitleText = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		TitleText->SetText(Title);
		Column->AddChildToVerticalBox(TitleText)->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
		Parent->AddChildToHorizontalBox(Column)->SetPadding(FMargin(16.f, 0.f));
		return Column;
	}
}


void UJ3InventoryWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	using namespace J3InventoryWidgetPrivate;

	// 화면 전체를 어둡게 덮는 배경
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
	WidgetTree->RootWidget = Root;
	UBorder* Dim = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Dim->SetBrushColor(DimColor);
	UCanvasPanelSlot* DimSlot = Root->AddChildToCanvas(Dim);
	DimSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	DimSlot->SetOffsets(FMargin(0.f));


	// 가운데: [장비 4칸 세로] [가방 격자]
	UHorizontalBox* Content = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UCanvasPanelSlot* ContentSlot = Root->AddChildToCanvas(Content);
	ContentSlot->SetAnchors(FAnchors(0.5f, 0.5f));
	ContentSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	ContentSlot->SetAutoSize(true);
	EquipmentBox = AddTitledColumn(WidgetTree, Content, LOCTEXT("Equipment", "장비"));
	UVerticalBox* BagColumn = AddTitledColumn(WidgetTree, Content, LOCTEXT("Bag", "가방"));
	BagGrid = WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass());
	BagGrid->SetSlotPadding(FMargin(2.f));
	BagColumn->AddChildToVerticalBox(BagGrid);


	// 장비 칸은 개수가 고정이다
	const int32 EquipmentCount = FMath::Max(1, UJ3DataSubsystem::Get()->GetConfigInt("equipment_slots", 4));
	for (int32 Index = 0; Index < EquipmentCount; ++Index)
	{
		UJ3ItemSlotWidget* SlotWidget = MakeSlot(EJ3SlotKind::Equipment, Index, FText::AsNumber(Index + 1));
		EquipmentBox->AddChildToVerticalBox(SlotWidget)->SetPadding(FMargin(0.f, 2.f));
		EquipmentSlots.Add(SlotWidget);
	}
}


void UJ3InventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 열려 있는 동안 인벤토리가 바뀌면 다시 그린다
	if (UJ3InventoryComponent* Inventory = GetInventory())
	{
		Inventory->OnInventoryChanged.AddUniqueDynamic(this, &UJ3InventoryWidget::Refresh);
	}
	Refresh();
}


void UJ3InventoryWidget::NativeDestruct()
{
	if (UJ3InventoryComponent* Inventory = GetInventory())
	{
		Inventory->OnInventoryChanged.RemoveDynamic(this, &UJ3InventoryWidget::Refresh);
	}
	Super::NativeDestruct();
}


UJ3InventoryComponent* UJ3InventoryWidget::GetInventory() const
{
	const AJ3PlayerState* PlayerState = GetOwningPlayerState<AJ3PlayerState>();
	return PlayerState != nullptr ? PlayerState->GetInventory() : nullptr;
}


UJ3ItemSlotWidget* UJ3InventoryWidget::MakeSlot(EJ3SlotKind Kind, int32 Index, const FText& CornerLabel)
{
	const TSubclassOf<UJ3ItemSlotWidget> SlotClass = SlotWidgetClass != nullptr ? SlotWidgetClass : TSubclassOf<UJ3ItemSlotWidget>(UJ3ItemSlotWidget::StaticClass());
	UJ3ItemSlotWidget* SlotWidget = CreateWidget<UJ3ItemSlotWidget>(GetOwningPlayer(), SlotClass);
	SlotWidget->Setup(Kind, Index, this, CornerLabel);
	return SlotWidget;
}


void UJ3InventoryWidget::RebuildBagSlots(int32 SlotCount)
{
	// 줄이 늘어나면(가방 장비) 칸을 다시 만든다
	const int32 Columns = FMath::Max(1, UJ3DataSubsystem::Get()->GetConfigInt("inventory_columns", 10));
	BagGrid->ClearChildren();
	BagSlots.Reset();
	for (int32 Index = 0; Index < SlotCount; ++Index)
	{
		UJ3ItemSlotWidget* SlotWidget = MakeSlot(EJ3SlotKind::Inventory, Index, FText::GetEmpty());
		BagGrid->AddChildToUniformGrid(SlotWidget, Index / Columns, Index % Columns);
		BagSlots.Add(SlotWidget);
	}
}


void UJ3InventoryWidget::Refresh()
{
	const UJ3InventoryComponent* Inventory = GetInventory();
	if (Inventory == nullptr)
	{
		return;
	}


	// 가방
	const TArray<FJ3ItemStack>& Slots = Inventory->GetSlots();
	if (BagSlots.Num() != Slots.Num())
	{
		RebuildBagSlots(Slots.Num());
	}
	for (int32 Index = 0; Index < Slots.Num(); ++Index)
	{
		BagSlots[Index]->SetItem(Slots[Index]);
	}


	// 장비 (손에 든 칸 강조)
	const TArray<FJ3ItemStack>& Equipment = Inventory->GetEquipment();
	for (int32 Index = 0; Index < EquipmentSlots.Num(); ++Index)
	{
		EquipmentSlots[Index]->SetItem(Equipment.IsValidIndex(Index) ? Equipment[Index] : FJ3ItemStack());
		EquipmentSlots[Index]->SetHighlighted(Index == Inventory->GetHeldEquipIndex());
	}
}


void UJ3InventoryWidget::HandleSlotClicked(EJ3SlotKind Kind, int32 Index)
{
	// 가방의 장비 클릭 → 위에서부터 장착, 장비 칸 클릭 → 가방으로 빼기 (확정)
	UJ3InventoryComponent* Inventory = GetInventory();
	if (Inventory == nullptr)
	{
		return;
	}
	if (Kind == EJ3SlotKind::Inventory)
	{
		Inventory->Server_EquipFromSlot(Index);
	}
	else
	{
		Inventory->Server_UnequipToInventory(Index);
	}
}


void UJ3InventoryWidget::HandleSlotDropped(EJ3SlotKind FromKind, int32 FromIndex, EJ3SlotKind ToKind, int32 ToIndex)
{
	UJ3InventoryComponent* Inventory = GetInventory();
	if (Inventory == nullptr)
	{
		return;
	}


	// 가방 → 가방: 옮기기·합치기 / 가방 → 장비: 위에서부터 장착 (확정: 드래그도 순서대로 장착)
	// 장비 → 장비: 순서 바꾸기 / 장비 → 가방: 빼기 (확정)
	if (FromKind == EJ3SlotKind::Inventory && ToKind == EJ3SlotKind::Inventory)
	{
		Inventory->Server_MoveSlot(FromIndex, ToIndex);
	}
	else if (FromKind == EJ3SlotKind::Inventory)
	{
		Inventory->Server_EquipFromSlot(FromIndex);
	}
	else if (ToKind == EJ3SlotKind::Equipment)
	{
		Inventory->Server_SwapEquipment(FromIndex, ToIndex);
	}
	else
	{
		Inventory->Server_UnequipToInventory(FromIndex);
	}
}

#undef LOCTEXT_NAMESPACE
