// Copyright (c) ACCENT. All Rights Reserved.

#include "UI/J3HUDWidget.h"

#include "Abilities/J3SurvivalAttributeSet.h"
#include "Blueprint/WidgetTree.h"
#include "Building/J3BuildModeComponent.h"
#include "Core/J3PlayerController.h"
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
#include "Player/J3Character.h"
#include "Player/J3HarvestComponent.h"
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
	BuildHarvestBar(Root);
	BuildBuildModeText(Root);
}


void UJ3HUDWidget::BuildBuildModeText(UCanvasPanel* Root)
{
	// 화면 아래 가운데
	BuildModeText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	BuildModeText->SetJustification(ETextJustify::Center);
	BuildModeText->SetVisibility(ESlateVisibility::Collapsed);
	J3HUDWidgetPrivate::Anchor(Root->AddChildToCanvas(BuildModeText), FVector2D(0.5f, 1.f), FVector2D(0.f, -ScreenMargin * 4.f));
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


void UJ3HUDWidget::BuildHarvestBar(UCanvasPanel* Root)
{
	// 조준점 아래: 채취 진행도
	USizeBox* Box = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	Box->SetWidthOverride(BarSize.X);
	Box->SetHeightOverride(BarSize.Y * 0.5f);
	HarvestBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass());
	HarvestBar->SetFillColorAndOpacity(HarvestColor);
	HarvestBar->SetVisibility(ESlateVisibility::Collapsed);
	Box->AddChild(HarvestBar);
	J3HUDWidgetPrivate::Anchor(Root->AddChildToCanvas(Box), FVector2D(0.5f, 0.5f), FVector2D(0.f, 40.f));
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


	// 건설 모드 안내
	const AJ3PlayerController* Controller = Cast<AJ3PlayerController>(GetOwningPlayer());
	const UJ3BuildModeComponent* BuildMode = Controller != nullptr ? Controller->GetBuildMode() : nullptr;
	if (BuildMode != nullptr && BuildMode->IsPlacing())
	{
		const FJ3BuildingDef* Def = UJ3DataSubsystem::Get()->GetBuilding(BuildMode->GetSelectedBuildingId());
		const FJ3PlacementCheck& Check = BuildMode->GetLastCheck();
		BuildModeText->SetText(FText::Format(LOCTEXT("BuildMode", "{0}\n{1}\n좌클릭: 설치   휠: 회전   가운데 클릭: 같은 건물   우클릭·Esc: 취소"),
			Def != nullptr ? Def->DisplayName : FText::GetEmpty(), J3Building::GetResultText(Check.Result)));
		BuildModeText->SetColorAndOpacity(Check.IsOk() ? FSlateColor(FLinearColor::White) : FSlateColor(FLinearColor(1.f, 0.35f, 0.3f)));
		BuildModeText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	else
	{
		BuildModeText->SetVisibility(ESlateVisibility::Collapsed);
	}


	// 채취 진행도
	const AJ3Character* Character = Cast<AJ3Character>(GetOwningPlayerPawn());
	const float HarvestProgress = Character != nullptr ? Character->GetHarvest()->GetProgress() : -1.f;
	HarvestBar->SetVisibility(HarvestProgress >= 0.f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	HarvestBar->SetPercent(FMath::Max(0.f, HarvestProgress));


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
