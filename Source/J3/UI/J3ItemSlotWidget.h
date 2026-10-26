// Copyright (c) ACCENT. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "Blueprint/UserWidget.h"
#include "Inventory/J3InventoryTypes.h"
#include "J3ItemSlotWidget.generated.h"

class UBorder;
class UJ3ItemSlotWidget;
class UTextBlock;


// 칸이 어디에 있는지 (인벤토리 가방 / 장비 칸 / 기계 칸 / 창고 칸, 기계 칸의 번호는 EJ3MachineSlot)
UENUM()
enum class EJ3SlotKind : uint8
{
	Inventory,
	Equipment,
	Machine,
	Storage,
};


// 클릭·드롭을 받아 처리하는 쪽 (인벤토리 화면). HUD의 칸처럼 표시만 하는 칸은 없어도 된다.
class IJ3ItemSlotHandler
{
public:
	virtual ~IJ3ItemSlotHandler() = default;
	virtual void HandleSlotClicked(EJ3SlotKind Kind, int32 Index) = 0;
	virtual void HandleSlotDropped(EJ3SlotKind FromKind, int32 FromIndex, EJ3SlotKind ToKind, int32 ToIndex) = 0;
};


// 드래그 중인 칸 정보
UCLASS()
class J3_API UJ3SlotDragOperation : public UDragDropOperation
{
	GENERATED_BODY()

public:
	EJ3SlotKind FromKind = EJ3SlotKind::Inventory;
	int32 FromIndex = INDEX_NONE;
};


// 아이템 칸 하나 (인벤토리·장비 공통). 위젯 구성은 코드로 만들고, 색·크기는 블루프린트에서 바꿀 수 있다.
UCLASS()
class J3_API UJ3ItemSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Setup(EJ3SlotKind InKind, int32 InIndex, IJ3ItemSlotHandler* InHandler, const FText& InCornerLabel);
	void SetItem(const FJ3ItemStack& InItem);
	void SetHighlighted(bool bInHighlighted);


	// 겉모습 (블루프린트에서 조정)
	UPROPERTY(EditDefaultsOnly, Category = "J3|Style")
	float SlotSize = 64.f;

	UPROPERTY(EditDefaultsOnly, Category = "J3|Style")
	FLinearColor EmptyColor = FLinearColor(0.f, 0.f, 0.f, 0.45f);

	UPROPERTY(EditDefaultsOnly, Category = "J3|Style")
	FLinearColor FilledColor = FLinearColor(0.15f, 0.15f, 0.15f, 0.8f);

	UPROPERTY(EditDefaultsOnly, Category = "J3|Style")
	FLinearColor HighlightColor = FLinearColor(0.95f, 0.75f, 0.2f, 0.9f);

protected:
	virtual void NativeOnInitialized() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

private:
	void RefreshVisual();

	UPROPERTY()
	TObjectPtr<UBorder> Background;

	UPROPERTY()
	TObjectPtr<UTextBlock> NameText;

	UPROPERTY()
	TObjectPtr<UTextBlock> CountText;

	UPROPERTY()
	TObjectPtr<UTextBlock> CornerText;

	EJ3SlotKind Kind = EJ3SlotKind::Inventory;
	int32 Index = INDEX_NONE;
	IJ3ItemSlotHandler* Handler = nullptr;
	FJ3ItemStack Item;
	bool bHighlighted = false;
	bool bPressed = false;
};
