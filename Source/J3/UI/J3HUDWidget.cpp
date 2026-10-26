// Copyright (c) ACCENT. All Rights Reserved.

#include "UI/J3HUDWidget.h"

#include "Abilities/J3SurvivalAttributeSet.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Core/J3PlayerState.h"
#include "Data/J3DataSubsystem.h"
#include "Inventory/J3InventoryComponent.h"
#include "UI/J3ItemSlotWidget.h"

#define LOCTEXT_NAMESPACE "J3HUD"


namespace J3HUDWidgetPrivate
{
	// 캔버스 모서리에 붙이고 크기는 내용에 맞춘다
	void Anchor(UCanvasPanelSlot* CanvasSlot, const FVector2D& AnchorPoint, const FVector2D& Offset)
	{
		CanvasSlot->SetAnchors(FAnchors(AnchorPoint.X, AnchorPoint.Y));
		CanvasSlot->SetAlignment(AnchorPoint);
		CanvasSlot->SetPosition(Offset);
		CanvasSlot->SetAutoSize(true);
	}
}


void UJ3HUDWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
	WidgetTree->RootWidget = Root;
	BuildStats(Root);
	BuildEquipment(Root);
	BuildCrosshair(Root);
}


void UJ3HUDWidget::BuildStats(UCanvasPanel* Root)
{
	// 왼쪽 아래: HP / 공복 / 갈증
	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	J3HUDWidgetPrivate::Anchor(Root->AddChildToCanvas(Box), FVector2D(0.f, 1.f), FVector2D(ScreenMargin, -ScreenMargin));
	HealthRow = AddStatRow(Box, LOCTEXT("Health", "HP"), HealthColor);
	HungerRow = AddStatRow(Box, LOCTEXT("Hunger", "공복"), HungerColor);
	ThirstRow = AddStatRow(Box, LOCTEXT("Thirst", "갈증"), ThirstColor);
}


UJ3HUDWidget::FStatRow UJ3HUDWidget::AddStatRow(UVerticalBox* Box, const FText& Label, const FLinearColor& Color)
{
	// [이름] [바] [현재/최대]
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Box->AddChildToVerticalBox(Row)->SetPadding(FMargin(0.f, 2.f));


	USizeBox* LabelBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	LabelBox->SetWidthOverride(40.f);
	UTextBlock* LabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	LabelText->SetText(Label);
	LabelBox->AddChild(LabelText);
	Row->AddChildToHorizontalBox(LabelBox)->SetVerticalAlignment(VAlign_Center);


	USizeBox* BarBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	BarBox->SetWidthOverride(BarSize.X);
	BarBox->SetHeightOverride(BarSize.Y);
	FStatRow Result;
	Result.Bar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass());
	Result.Bar->SetFillColorAndOpacity(Color);
	BarBox->AddChild(Result.Bar);
	Row->AddChildToHorizontalBox(BarBox)->SetVerticalAlignment(VAlign_Center);


	Result.Value = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	UHorizontalBoxSlot* ValueSlot = Row->AddChildToHorizontalBox(Result.Value);
	ValueSlot->SetPadding(FMargin(8.f, 0.f, 0.f, 0.f));
	ValueSlot->SetVerticalAlignment(VAlign_Center);
	return Result;
}


void UJ3HUDWidget::BuildEquipment(UCanvasPanel* Root)
{
	// 오른쪽 아래: 장비 칸 (번호 표시, 손에 든 칸 강조)
	UHorizontalBox* Box = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	J3HUDWidgetPrivate::Anchor(Root->AddChildToCanvas(Box), FVector2D(1.f, 1.f), FVector2D(-ScreenMargin, -ScreenMargin));
	const TSubclassOf<UJ3ItemSlotWidget> SlotClass = SlotWidgetClass != nullptr ? SlotWidgetClass : TSubclassOf<UJ3ItemSlotWidget>(UJ3ItemSlotWidget::StaticClass());
	const int32 SlotCount = FMath::Max(1, UJ3DataSubsystem::Get()->GetConfigInt("equipment_slots", 4));
	for (int32 Index = 0; Index < SlotCount; ++Index)
	{
		UJ3ItemSlotWidget* SlotWidget = CreateWidget<UJ3ItemSlotWidget>(GetOwningPlayer(), SlotClass);
		SlotWidget->Setup(EJ3SlotKind::Equipment, Index, nullptr, FText::AsNumber(Index + 1));
		Box->AddChildToHorizontalBox(SlotWidget)->SetPadding(FMargin(2.f, 0.f));
		EquipmentSlots.Add(SlotWidget);
	}
}


void UJ3HUDWidget::BuildCrosshair(UCanvasPanel* Root)
{
	// 가운데 조준점. TODO(기획-제안): 무기를 들었을 때만 진하게
	UTextBlock* Crosshair = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Crosshair->SetText(FText::FromString(TEXT("+")));
	J3HUDWidgetPrivate::Anchor(Root->AddChildToCanvas(Crosshair), FVector2D(0.5f, 0.5f), FVector2D::ZeroVector);
}


void UJ3HUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// PlayerState는 접속 직후 아직 복제되지 않았을 수 있다
	const AJ3PlayerState* PlayerState = GetOwningPlayerState<AJ3PlayerState>();
	const UJ3SurvivalAttributeSet* Attributes = PlayerState != nullptr ? PlayerState->GetSurvivalAttributes() : nullptr;
	if (Attributes == nullptr)
	{
		return;
	}
	SetStat(HealthRow, Attributes->GetHealth(), Attributes->GetMaxHealth());
	SetStat(HungerRow, Attributes->GetHunger(), Attributes->GetMaxHunger());
	SetStat(ThirstRow, Attributes->GetThirst(), Attributes->GetMaxThirst());


	// 장비 칸
	const UJ3InventoryComponent* Inventory = PlayerState->GetInventory();
	const TArray<FJ3ItemStack>& Equipment = Inventory->GetEquipment();
	for (int32 Index = 0; Index < EquipmentSlots.Num(); ++Index)
	{
		EquipmentSlots[Index]->SetItem(Equipment.IsValidIndex(Index) ? Equipment[Index] : FJ3ItemStack());
		EquipmentSlots[Index]->SetHighlighted(Index == Inventory->GetHeldEquipIndex());
	}
}


void UJ3HUDWidget::SetStat(const FStatRow& Row, float Current, float Max)
{
	// "현재/최대" (소수점은 버려 바가 떨리지 않게)
	Row.Bar->SetPercent(Max > 0.f ? Current / Max : 0.f);
	Row.Value->SetText(FText::Format(LOCTEXT("StatValue", "{0}/{1}"), FText::AsNumber(FMath::FloorToInt(Current)), FText::AsNumber(FMath::RoundToInt(Max))));
}

#undef LOCTEXT_NAMESPACE
