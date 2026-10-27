// Copyright (c) ACCENT. All Rights Reserved.

#include "UI/J3MachineWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Building/J3MachineAccessComponent.h"
#include "Building/J3MachineBuildings.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Core/J3PlayerController.h"
#include "Core/J3PlayerState.h"
#include "Data/J3DataSubsystem.h"
#include "Inventory/J3InventoryComponent.h"

#define LOCTEXT_NAMESPACE "J3Machine"


namespace J3MachineWidgetPrivate
{
	FText GetStatusText(EJ3MachineStatus Status)
	{
		switch (Status)
		{
		case EJ3MachineStatus::Running:
			return LOCTEXT("Running", "가동 중");
		case EJ3MachineStatus::OutputFull:
			return LOCTEXT("OutputFull", "멈춤: 출력 칸이 가득 찼습니다 (꺼내 가세요)");
		case EJ3MachineStatus::NoInput:
			return LOCTEXT("NoInput", "멈춤: 재료가 없습니다");
		case EJ3MachineStatus::NoFuel:
			return LOCTEXT("NoFuel", "멈춤: 연료가 없습니다");
		case EJ3MachineStatus::NoResource:
			return LOCTEXT("NoResource", "멈춤: 아래에 캘 자원이 없습니다");
		case EJ3MachineStatus::NoPower:
			return LOCTEXT("NoPower", "멈춤: 전력이 부족합니다 (발전소 연료·전기줄 확인)");
		}
		return FText::GetEmpty();
	}
}


void UJ3MachineWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// 배경을 어둡게 덮고 가운데에 [제목, 상태, 진행 막대, 기계 칸, 가방]
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
	Column->AddChildToVerticalBox(TitleText);
	StatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Column->AddChildToVerticalBox(StatusText)->SetPadding(FMargin(0.f, 4.f));
	USizeBox* BarBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	BarBox->SetHeightOverride(10.f);
	ProgressBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass());
	BarBox->AddChild(ProgressBar);
	Column->AddChildToVerticalBox(BarBox)->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
	MachineSlotBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Column->AddChildToVerticalBox(MachineSlotBox)->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));


	UTextBlock* BagLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	BagLabel->SetText(LOCTEXT("Bag", "가방"));
	Column->AddChildToVerticalBox(BagLabel);
	BagGrid = WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass());
	BagGrid->SetSlotPadding(FMargin(2.f));
	Column->AddChildToVerticalBox(BagGrid);
	UTextBlock* Hint = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Hint->SetText(LOCTEXT("Hint", "가방 칸 클릭: 넣기   기계 칸 클릭: 꺼내기   Esc: 닫기"));
	Column->AddChildToVerticalBox(Hint)->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));
	SetIsFocusable(true);
}


UJ3ItemSlotWidget* UJ3MachineWidget::MakeSlot(EJ3SlotKind Kind, int32 Index, const FText& CornerLabel)
{
	const TSubclassOf<UJ3ItemSlotWidget> SlotClass = SlotWidgetClass != nullptr ? SlotWidgetClass : TSubclassOf<UJ3ItemSlotWidget>(UJ3ItemSlotWidget::StaticClass());
	UJ3ItemSlotWidget* SlotWidget = CreateWidget<UJ3ItemSlotWidget>(GetOwningPlayer(), SlotClass);
	SlotWidget->Setup(Kind, Index, this, CornerLabel);
	return SlotWidget;
}


void UJ3MachineWidget::SetMachine(AJ3MachineBuilding* InMachine)
{
	// 기계마다 있는 칸이 다르다 (상한 0인 칸은 없다)
	Machine = InMachine;
	MachineSlotBox->ClearChildren();
	MachineSlots.Reset();
	MachineSlots.SetNum(3);
	const FJ3MachineDef* Def = InMachine != nullptr ? InMachine->GetMachineDef() : nullptr;
	const FJ3BuildingDef* BuildingDef = InMachine != nullptr ? InMachine->GetDef() : nullptr;
	TitleText->SetText(BuildingDef != nullptr ? BuildingDef->DisplayName : FText::GetEmpty());
	if (Def == nullptr)
	{
		return;
	}
	const TPair<EJ3MachineSlot, FText> Layout[] = {
		{ EJ3MachineSlot::Input, LOCTEXT("Input", "재료") },
		{ EJ3MachineSlot::Fuel, Def->PowerUse > 0.f ? LOCTEXT("Battery", "배터리") : LOCTEXT("Fuel", "연료") },
		{ EJ3MachineSlot::Output, LOCTEXT("Output", "결과") } };
	const int32 Caps[] = { Def->InputCap, Def->FuelCap, Def->OutputCap };
	for (const TPair<EJ3MachineSlot, FText>& Entry : Layout)
	{
		const int32 SlotIndex = static_cast<int32>(Entry.Key);
		if (Caps[SlotIndex] <= 0)
		{
			continue;
		}
		UJ3ItemSlotWidget* SlotWidget = MakeSlot(EJ3SlotKind::Machine, SlotIndex, Entry.Value);
		MachineSlotBox->AddChildToHorizontalBox(SlotWidget)->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
		MachineSlots[SlotIndex] = SlotWidget;
	}
	Refresh();
}


void UJ3MachineWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 기계가 사라지거나 멀어지면 닫는다
	const AJ3PlayerState* PlayerState = GetOwningPlayerState<AJ3PlayerState>();
	AJ3PlayerController* Controller = Cast<AJ3PlayerController>(GetOwningPlayer());
	if (PlayerState != nullptr && Controller != nullptr && !PlayerState->GetMachineAccess()->IsNearMachine(Machine.Get()))
	{
		Controller->CloseMachineScreen();
		return;
	}
	Refresh();
}


FReply UJ3MachineWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		if (AJ3PlayerController* Controller = Cast<AJ3PlayerController>(GetOwningPlayer()))
		{
			Controller->CloseMachineScreen();
		}
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}


void UJ3MachineWidget::Refresh()
{
	using namespace J3MachineWidgetPrivate;

	const AJ3PlayerState* PlayerState = GetOwningPlayerState<AJ3PlayerState>();
	const AJ3MachineBuilding* CurrentMachine = Machine.Get();
	if (PlayerState == nullptr || CurrentMachine == nullptr)
	{
		return;
	}


	// 기계 칸과 상태 (복제된 값)
	const FJ3MachineView& View = CurrentMachine->GetView();
	for (int32 Index = 0; Index < MachineSlots.Num(); ++Index)
	{
		if (MachineSlots[Index] != nullptr)
		{
			MachineSlots[Index]->SetItem(View.GetSlot(static_cast<EJ3MachineSlot>(Index)));
		}
	}
	const FJ3MachineDef* Def = CurrentMachine->GetMachineDef();
	const bool bGenerator = Def != nullptr && Def->IsGenerator();
	if (bGenerator && View.Status == EJ3MachineStatus::Running)
	{
		StatusText->SetText(FText::Format(LOCTEXT("Generating", "발전 중 (축전 {0}%)"), FMath::FloorToInt(View.EnergyFraction * 100.f)));
	}
	else if (bGenerator && View.Status == EJ3MachineStatus::OutputFull)
	{
		StatusText->SetText(LOCTEXT("EnergyFull", "축전이 가득 찼습니다 (쓰는 만큼 다시 발전)"));
	}
	else
	{
		StatusText->SetText(GetStatusText(View.Status));
	}
	ProgressBar->SetPercent(FMath::Max(0.f, CurrentMachine->GetProgress()));


	// 가방 (줄이 늘면 다시 만든다)
	const TArray<FJ3ItemStack>& Slots = PlayerState->GetInventory()->GetSlots();
	if (BagSlots.Num() != Slots.Num())
	{
		const int32 Columns = FMath::Max(1, UJ3DataSubsystem::Get()->GetConfigInt("inventory_columns", 10));
		BagGrid->ClearChildren();
		BagSlots.Reset();
		for (int32 Index = 0; Index < Slots.Num(); ++Index)
		{
			UJ3ItemSlotWidget* SlotWidget = MakeSlot(EJ3SlotKind::Inventory, Index, FText::GetEmpty());
			BagGrid->AddChildToUniformGrid(SlotWidget, Index / Columns, Index % Columns);
			BagSlots.Add(SlotWidget);
		}
	}
	for (int32 Index = 0; Index < Slots.Num(); ++Index)
	{
		BagSlots[Index]->SetItem(Slots[Index]);
	}
}


void UJ3MachineWidget::HandleSlotClicked(EJ3SlotKind Kind, int32 Index)
{
	// 가방 → 기계에 넣기, 기계 → 가방으로 꺼내기 (판정은 서버)
	const AJ3PlayerState* PlayerState = GetOwningPlayerState<AJ3PlayerState>();
	if (PlayerState == nullptr || !Machine.IsValid())
	{
		return;
	}
	if (Kind == EJ3SlotKind::Inventory)
	{
		PlayerState->GetMachineAccess()->Server_InsertFromInventory(Machine.Get(), Index);
	}
	else if (Kind == EJ3SlotKind::Machine)
	{
		PlayerState->GetMachineAccess()->Server_TakeToInventory(Machine.Get(), static_cast<EJ3MachineSlot>(Index));
	}
}


void UJ3MachineWidget::HandleSlotDropped(EJ3SlotKind FromKind, int32 FromIndex, EJ3SlotKind ToKind, int32 ToIndex)
{
	// 끌어 놓기도 클릭과 같다 (가방 → 기계: 넣기, 기계 → 가방: 꺼내기, 가방 → 가방: 옮기기)
	const AJ3PlayerState* PlayerState = GetOwningPlayerState<AJ3PlayerState>();
	if (PlayerState == nullptr)
	{
		return;
	}
	if (FromKind == EJ3SlotKind::Inventory && ToKind == EJ3SlotKind::Inventory)
	{
		PlayerState->GetInventory()->Server_MoveSlot(FromIndex, ToIndex);
		return;
	}
	if (FromKind != ToKind)
	{
		HandleSlotClicked(FromKind, FromIndex);
	}
}

#undef LOCTEXT_NAMESPACE
