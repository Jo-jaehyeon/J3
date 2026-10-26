// Copyright (c) ACCENT. All Rights Reserved.

#include "UI/J3ItemSlotWidget.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Data/J3DataSubsystem.h"


namespace J3ItemSlotWidgetPrivate
{
	UTextBlock* AddText(UWidgetTree* Tree, UOverlay* Overlay, int32 FontSize, EHorizontalAlignment HAlign, EVerticalAlignment VAlign)
	{
		UTextBlock* Text = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		FSlateFontInfo Font = Text->GetFont();
		Font.Size = FontSize;
		Text->SetFont(Font);
		Text->SetAutoWrapText(true);
		UOverlaySlot* OverlaySlot = Overlay->AddChildToOverlay(Text);
		OverlaySlot->SetHorizontalAlignment(HAlign);
		OverlaySlot->SetVerticalAlignment(VAlign);
		OverlaySlot->SetPadding(FMargin(3.f));
		return Text;
	}
}


void UJ3ItemSlotWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	using namespace J3ItemSlotWidgetPrivate;

	// 크기 고정 상자 > 배경 > (이름, 수량, 칸 번호)
	USizeBox* Box = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	Box->SetWidthOverride(SlotSize);
	Box->SetHeightOverride(SlotSize);
	WidgetTree->RootWidget = Box;
	Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Box->AddChild(Background);
	UOverlay* Overlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	Background->SetContent(Overlay);


	NameText = AddText(WidgetTree, Overlay, 9, HAlign_Center, VAlign_Center);
	NameText->SetJustification(ETextJustify::Center);
	CountText = AddText(WidgetTree, Overlay, 9, HAlign_Right, VAlign_Bottom);
	CornerText = AddText(WidgetTree, Overlay, 8, HAlign_Left, VAlign_Top);
	RefreshVisual();
}


void UJ3ItemSlotWidget::Setup(EJ3SlotKind InKind, int32 InIndex, IJ3ItemSlotHandler* InHandler, const FText& InCornerLabel)
{
	Kind = InKind;
	Index = InIndex;
	Handler = InHandler;
	CornerText->SetText(InCornerLabel);
}


void UJ3ItemSlotWidget::SetItem(const FJ3ItemStack& InItem)
{
	if (Item == InItem)
	{
		return;
	}
	Item = InItem;
	RefreshVisual();
}


void UJ3ItemSlotWidget::SetHighlighted(bool bInHighlighted)
{
	if (bHighlighted == bInHighlighted)
	{
		return;
	}
	bHighlighted = bInHighlighted;
	RefreshVisual();
}


void UJ3ItemSlotWidget::RefreshVisual()
{
	if (Background == nullptr)
	{
		return;
	}


	// 배경색: 강조 > 채움 > 빈 칸
	const FLinearColor Color = bHighlighted ? HighlightColor : (Item.IsEmpty() ? EmptyColor : FilledColor);
	Background->SetBrushColor(Color);


	// 이름(데이터의 표시 이름)과 수량. TODO(아트): 아이템 아이콘
	if (Item.IsEmpty())
	{
		NameText->SetText(FText::GetEmpty());
		CountText->SetText(FText::GetEmpty());
		return;
	}
	const FJ3ItemDef* Def = UJ3DataSubsystem::Get()->GetItem(Item.ItemId);
	NameText->SetText(Def != nullptr ? Def->DisplayName : FText::FromName(Item.ItemId));
	CountText->SetText(Item.Count > 1 ? FText::AsNumber(Item.Count) : FText::GetEmpty());
}


FReply UJ3ItemSlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// 좌클릭: 떼면 클릭, 끌면 드래그 (Player_And_Survival 1절: 좌클릭은 UI 안 드래그·장착)
	if (Handler == nullptr || InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return FReply::Unhandled();
	}
	bPressed = true;
	return UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent, this, EKeys::LeftMouseButton).NativeReply;
}


FReply UJ3ItemSlotWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// 드래그 없이 뗐으면 클릭
	if (!bPressed || InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return FReply::Unhandled();
	}
	bPressed = false;
	if (Handler != nullptr && !Item.IsEmpty())
	{
		Handler->HandleSlotClicked(Kind, Index);
	}
	return FReply::Handled();
}


void UJ3ItemSlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	// 빈 칸은 끌 수 없다
	bPressed = false;
	if (Item.IsEmpty())
	{
		return;
	}


	// 끄는 동안 보이는 칸 복사본
	UJ3ItemSlotWidget* DragVisual = CreateWidget<UJ3ItemSlotWidget>(GetOwningPlayer(), GetClass());
	DragVisual->SetItem(Item);
	UJ3SlotDragOperation* Operation = NewObject<UJ3SlotDragOperation>(this);
	Operation->FromKind = Kind;
	Operation->FromIndex = Index;
	Operation->DefaultDragVisual = DragVisual;
	Operation->Pivot = EDragPivot::CenterCenter;
	OutOperation = Operation;
}


bool UJ3ItemSlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	const UJ3SlotDragOperation* Operation = Cast<UJ3SlotDragOperation>(InOperation);
	if (Handler == nullptr || Operation == nullptr)
	{
		return false;
	}
	Handler->HandleSlotDropped(Operation->FromKind, Operation->FromIndex, Kind, Index);
	return true;
}
