// Copyright (c) ACCENT. All Rights Reserved.

#include "UI/J3CraftingWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Core/J3PlayerController.h"
#include "Core/J3PlayerState.h"
#include "Crafting/J3CraftingComponent.h"
#include "Crafting/J3CraftingStationComponent.h"
#include "Data/J3DataSubsystem.h"
#include "Inventory/J3InventoryComponent.h"

#define LOCTEXT_NAMESPACE "J3Crafting"


namespace J3CraftingWidgetPrivate
{
	FText ItemName(FName ItemId)
	{
		const FJ3ItemDef* Item = UJ3DataSubsystem::Get()->GetItem(ItemId);
		return Item != nullptr ? Item->DisplayName : FText::FromName(ItemId);
	}
}


void UJ3RecipeRowWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// [결과물 이름 (고정 폭)] [재료 목록]
	Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Background->SetPadding(FMargin(8.f, 4.f));
	WidgetTree->RootWidget = Background;
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Background->SetContent(Row);
	USizeBox* NameBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	NameBox->SetWidthOverride(140.f);
	OutputLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	NameBox->AddChild(OutputLabel);
	Row->AddChildToHorizontalBox(NameBox)->SetVerticalAlignment(VAlign_Center);
	InputsLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Row->AddChildToHorizontalBox(InputsLabel)->SetVerticalAlignment(VAlign_Center);
}


void UJ3RecipeRowWidget::Setup(UJ3CraftingWidget* InOwner, FName InRecipeId)
{
	Owner = InOwner;
	RecipeId = InRecipeId;
}


void UJ3RecipeRowWidget::Refresh(bool bCanCraft, const FText& OutputText, const FText& InputsText)
{
	Background->SetBrushColor(bCanCraft ? ReadyColor : MissingColor);
	OutputLabel->SetText(OutputText);
	InputsLabel->SetText(InputsText);
}


FReply UJ3RecipeRowWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton || !Owner.IsValid())
	{
		return FReply::Unhandled();
	}
	Owner->RequestCraft(RecipeId);
	return FReply::Handled();
}


void UJ3CraftingWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// 배경을 어둡게 덮고 가운데에 [제목, 목록]
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
	Column->AddChildToVerticalBox(TitleText)->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
	RecipeList = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Column->AddChildToVerticalBox(RecipeList);
	UTextBlock* Hint = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Hint->SetText(LOCTEXT("Hint", "클릭: 제작   Esc: 닫기"));
	Column->AddChildToVerticalBox(Hint)->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));
	SetIsFocusable(true);
}


void UJ3CraftingWidget::SetStation(AActor* InStation)
{
	Station = InStation;
	Rows.Reset();
	RecipeList->ClearChildren();
	const UJ3CraftingStationComponent* StationComponent = InStation != nullptr ? InStation->FindComponentByClass<UJ3CraftingStationComponent>() : nullptr;
	TitleText->SetText(FText::Format(LOCTEXT("Title", "제작 ({0})"), FText::FromName(StationComponent != nullptr ? StationComponent->GetStationId() : NAME_None)));
	Refresh();
}


void UJ3CraftingWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 장소에서 멀어지면 닫는다
	const AJ3PlayerState* PlayerState = GetOwningPlayerState<AJ3PlayerState>();
	AJ3PlayerController* Controller = Cast<AJ3PlayerController>(GetOwningPlayer());
	if (PlayerState != nullptr && Controller != nullptr && !PlayerState->GetCrafting()->IsNearStation(Station.Get()))
	{
		Controller->CloseCraftingScreen();
		return;
	}
	Refresh();
}


FReply UJ3CraftingWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		if (AJ3PlayerController* Controller = Cast<AJ3PlayerController>(GetOwningPlayer()))
		{
			Controller->CloseCraftingScreen();
		}
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}


void UJ3CraftingWidget::Refresh()
{
	using namespace J3CraftingWidgetPrivate;

	const AJ3PlayerState* PlayerState = GetOwningPlayerState<AJ3PlayerState>();
	if (PlayerState == nullptr)
	{
		return;
	}
	const UJ3CraftingComponent* Crafting = PlayerState->GetCrafting();
	const UJ3InventoryComponent* Inventory = PlayerState->GetInventory();


	// 이 장소에서 해방된 레시피만, 레시피 ID 순서로
	TArray<FName> RecipeIds;
	for (const TPair<FName, FJ3RecipeDef>& Pair : UJ3DataSubsystem::Get()->GetAllRecipes())
	{
		if (Crafting->IsRecipeAvailableAt(Pair.Value, Station.Get()))
		{
			RecipeIds.Add(Pair.Key);
		}
	}
	RecipeIds.Sort(FNameLexicalLess());


	// 줄 만들기·갱신: "돌 1/1, 나뭇가지 0/1"
	for (const FName& RecipeId : RecipeIds)
	{
		const FJ3RecipeDef* Recipe = UJ3DataSubsystem::Get()->GetRecipe(RecipeId);
		TObjectPtr<UJ3RecipeRowWidget>& RowWidget = Rows.FindOrAdd(RecipeId);
		if (RowWidget == nullptr)
		{
			const TSubclassOf<UJ3RecipeRowWidget> RowClass = RowWidgetClass != nullptr ? RowWidgetClass : TSubclassOf<UJ3RecipeRowWidget>(UJ3RecipeRowWidget::StaticClass());
			RowWidget = CreateWidget<UJ3RecipeRowWidget>(GetOwningPlayer(), RowClass);
			RowWidget->Setup(this, RecipeId);
			RecipeList->AddChildToVerticalBox(RowWidget)->SetPadding(FMargin(0.f, 2.f));
		}
		TArray<FString> Parts;
		bool bCanCraft = true;
		for (const FJ3ItemAmount& Input : Recipe->Inputs)
		{
			const int32 Owned = Inventory->CountItem(Input.ItemId);
			bCanCraft &= Owned >= Input.Count;
			Parts.Add(FString::Printf(TEXT("%s %d/%d"), *ItemName(Input.ItemId).ToString(), Owned, Input.Count));
		}
		const FText OutputText = Recipe->Output.Count > 1 ? FText::Format(LOCTEXT("OutputCount", "{0} x{1}"), ItemName(Recipe->Output.ItemId), Recipe->Output.Count) : ItemName(Recipe->Output.ItemId);
		RowWidget->Refresh(bCanCraft, OutputText, FText::FromString(FString::Join(Parts, TEXT(", "))));
	}
}


void UJ3CraftingWidget::RequestCraft(FName RecipeId)
{
	const AJ3PlayerState* PlayerState = GetOwningPlayerState<AJ3PlayerState>();
	if (PlayerState != nullptr)
	{
		PlayerState->GetCrafting()->Server_Craft(Station.Get(), RecipeId);
	}
}

#undef LOCTEXT_NAMESPACE
