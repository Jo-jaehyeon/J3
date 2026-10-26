// Copyright (c) ACCENT. All Rights Reserved.

#include "UI/J3BuildMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Building/J3BuildModeComponent.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Core/J3GameState.h"
#include "Core/J3PlayerController.h"
#include "Core/J3PlayerState.h"
#include "Data/J3DataSubsystem.h"
#include "Inventory/J3InventoryComponent.h"

#define LOCTEXT_NAMESPACE "J3BuildMenu"


namespace J3BuildMenuWidgetPrivate
{
	// 탭 순서와 이름 (건설 메뉴 분류, Inventory_And_UI 1.3)
	const TArray<TPair<FName, FText>>& GetTabs()
	{
		static const TArray<TPair<FName, FText>> Tabs = {
			{ "production", LOCTEXT("Production", "생산") },
			{ "logistics", LOCTEXT("Logistics", "물류") },
			{ "power", LOCTEXT("Power", "전력") },
			{ "structure", LOCTEXT("Structure", "구조물") },
			{ "base", LOCTEXT("Base", "기지") },
		};
		return Tabs;
	}


	// 숫자키 1~0 → 단축 칸 0~9
	int32 HotbarIndexOf(const FKey& Key)
	{
		static const FKey Keys[] = { EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five, EKeys::Six, EKeys::Seven, EKeys::Eight, EKeys::Nine, EKeys::Zero };
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(Keys); ++Index)
		{
			if (Keys[Index] == Key)
			{
				return Index;
			}
		}
		return INDEX_NONE;
	}


	FText ItemName(FName ItemId)
	{
		const FJ3ItemDef* Item = UJ3DataSubsystem::Get()->GetItem(ItemId);
		return Item != nullptr ? Item->DisplayName : FText::FromName(ItemId);
	}
}


void UJ3BuildMenuEntryWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// [이름] [자세히]
	Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Background->SetPadding(FMargin(10.f, 5.f));
	WidgetTree->RootWidget = Background;
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Background->SetContent(Row);
	USizeBox* NameBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	NameBox->SetMinDesiredWidth(90.f);
	LabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	NameBox->AddChild(LabelText);
	Row->AddChildToHorizontalBox(NameBox)->SetVerticalAlignment(VAlign_Center);
	DetailText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Row->AddChildToHorizontalBox(DetailText)->SetVerticalAlignment(VAlign_Center);
}


void UJ3BuildMenuEntryWidget::Setup(UJ3BuildMenuWidget* InOwner, FName InKey, bool bInIsTab)
{
	Owner = InOwner;
	Key = InKey;
	bIsTab = bInIsTab;
}


void UJ3BuildMenuEntryWidget::Refresh(const FText& Label, const FText& Detail, const FLinearColor& Color)
{
	LabelText->SetText(Label);
	DetailText->SetText(Detail);
	Background->SetBrushColor(Color);
}


FReply UJ3BuildMenuEntryWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton || !Owner.IsValid())
	{
		return FReply::Unhandled();
	}
	Owner->HandleEntryClicked(Key, bIsTab);
	return FReply::Handled();
}


void UJ3BuildMenuEntryWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
	if (Owner.IsValid())
	{
		Owner->HandleEntryHovered(Key, bIsTab, true);
	}
}


void UJ3BuildMenuEntryWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
	if (Owner.IsValid())
	{
		Owner->HandleEntryHovered(Key, bIsTab, false);
	}
}


void UJ3BuildMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	using namespace J3BuildMenuWidgetPrivate;

	// 배경을 어둡게 덮고 가운데에 [탭 줄] [건물 목록] [안내]
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


	TabRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Column->AddChildToVerticalBox(TabRow)->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
	for (const TPair<FName, FText>& Tab : GetTabs())
	{
		UJ3BuildMenuEntryWidget* Entry = MakeEntry(Tab.Key, true);
		TabRow->AddChildToHorizontalBox(Entry)->SetPadding(FMargin(2.f, 0.f));
		Tabs.Add(Tab.Key, Entry);
	}
	BuildingList = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Column->AddChildToVerticalBox(BuildingList);
	UTextBlock* Hint = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Hint->SetText(LOCTEXT("Hint", "클릭: 건설   마우스를 올리고 숫자키: 단축 칸 등록   Q·Esc: 닫기"));
	Column->AddChildToVerticalBox(Hint)->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));
	SetIsFocusable(true);
}


void UJ3BuildMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	Refresh();
}


UJ3BuildMenuEntryWidget* UJ3BuildMenuWidget::MakeEntry(FName Key, bool bIsTab)
{
	UJ3BuildMenuEntryWidget* Entry = CreateWidget<UJ3BuildMenuEntryWidget>(GetOwningPlayer(), UJ3BuildMenuEntryWidget::StaticClass());
	Entry->Setup(this, Key, bIsTab);
	return Entry;
}


void UJ3BuildMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	Refresh();
}


void UJ3BuildMenuWidget::Refresh()
{
	using namespace J3BuildMenuWidgetPrivate;

	const AJ3PlayerState* PlayerState = GetOwningPlayerState<AJ3PlayerState>();
	const AJ3GameState* GameState = GetWorld()->GetGameState<AJ3GameState>();
	const AJ3PlayerController* Controller = Cast<AJ3PlayerController>(GetOwningPlayer());
	if (PlayerState == nullptr || GameState == nullptr || Controller == nullptr)
	{
		return;
	}


	// 탭 (고른 탭 강조)
	for (const TPair<FName, FText>& Tab : GetTabs())
	{
		Tabs[Tab.Key]->Refresh(Tab.Value, FText::GetEmpty(), Tab.Key == SelectedTab ? SelectedTabColor : TabColor);
	}


	// 이 탭의 해방된 건물 (ID 순서)
	TArray<FName> BuildingIds;
	for (const TPair<FName, FJ3BuildingDef>& Pair : UJ3DataSubsystem::Get()->GetAllBuildings())
	{
		if (Pair.Value.Category == SelectedTab && GameState->IsUnlocked(Pair.Value.Unlock))
		{
			BuildingIds.Add(Pair.Key);
		}
	}
	BuildingIds.Sort(FNameLexicalLess());


	// 줄 맞추기: 탭이 바뀌면 다른 건물 줄은 숨긴다
	for (TPair<FName, TObjectPtr<UJ3BuildMenuEntryWidget>>& Pair : Rows)
	{
		Pair.Value->SetVisibility(BuildingIds.Contains(Pair.Key) ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	for (const FName& BuildingId : BuildingIds)
	{
		TObjectPtr<UJ3BuildMenuEntryWidget>& Row = Rows.FindOrAdd(BuildingId);
		if (Row == nullptr)
		{
			Row = MakeEntry(BuildingId, false);
			BuildingList->AddChildToVerticalBox(Row)->SetPadding(FMargin(0.f, 2.f));
		}


		// "돌 1/1, 나뭇가지 0/1  [3]"
		const FJ3BuildingDef* Def = UJ3DataSubsystem::Get()->GetBuilding(BuildingId);
		TArray<FString> Parts;
		bool bReady = true;
		for (const FJ3ItemAmount& Input : Def->BuildInputs)
		{
			const int32 Owned = PlayerState->GetInventory()->CountItem(Input.ItemId);
			bReady &= Owned >= Input.Count;
			Parts.Add(FString::Printf(TEXT("%s %d/%d"), *ItemName(Input.ItemId).ToString(), Owned, Input.Count));
		}
		for (int32 Index = 0; Index < 10; ++Index)
		{
			if (Controller->GetBuildMode()->GetHotbar(Index) == BuildingId)
			{
				Parts.Add(FString::Printf(TEXT("[%d]"), (Index + 1) % 10));
			}
		}
		Row->Refresh(Def->DisplayName, FText::FromString(FString::Join(Parts, TEXT(", "))), bReady ? ReadyColor : MissingColor);
	}
}


void UJ3BuildMenuWidget::HandleEntryClicked(FName Key, bool bIsTab)
{
	// 탭: 바꾸기 / 건물: 건설 모드로 들어가며 메뉴를 닫는다
	if (bIsTab)
	{
		SelectedTab = Key;
		Refresh();
		return;
	}
	if (AJ3PlayerController* Controller = Cast<AJ3PlayerController>(GetOwningPlayer()))
	{
		Controller->CloseBuildMenu();
		Controller->GetBuildMode()->BeginPlacing(Key);
	}
}


void UJ3BuildMenuWidget::HandleEntryHovered(FName Key, bool bIsTab, bool bHovered)
{
	if (bIsTab)
	{
		return;
	}
	if (bHovered)
	{
		HoveredBuilding = Key;
	}
	else if (HoveredBuilding == Key)
	{
		HoveredBuilding = NAME_None;
	}
}


FReply UJ3BuildMenuWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	AJ3PlayerController* Controller = Cast<AJ3PlayerController>(GetOwningPlayer());
	if (Controller == nullptr)
	{
		return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
	}


	// 마우스를 올린 건물 + 숫자키 = 단축 칸 등록
	const int32 HotbarIndex = J3BuildMenuWidgetPrivate::HotbarIndexOf(InKeyEvent.GetKey());
	if (HotbarIndex != INDEX_NONE && !HoveredBuilding.IsNone())
	{
		Controller->GetBuildMode()->AssignHotbar(HotbarIndex, HoveredBuilding);
		Refresh();
		return FReply::Handled();
	}


	// Esc로 닫기 (Q는 게임 입력으로 들어가 컨트롤러가 닫는다)
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		Controller->CloseBuildMenu();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

#undef LOCTEXT_NAMESPACE
