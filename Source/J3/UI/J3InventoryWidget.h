// Copyright (c) ACCENT. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/J3ItemSlotWidget.h"
#include "J3InventoryWidget.generated.h"

class UJ3InventoryComponent;
class UUniformGridPanel;
class UVerticalBox;


// 인벤토리 화면 (I키, Inventory_And_UI 1절): 왼쪽 장비 4칸, 오른쪽 10×N 가방.
// 클릭·드래그는 서버 요청(Server_ RPC)으로 보내고, 결과는 복제로 받아 다시 그린다.
UCLASS()
class J3_API UJ3InventoryWidget : public UUserWidget, public IJ3ItemSlotHandler
{
	GENERATED_BODY()

public:
	// 겉모습 (블루프린트에서 조정)
	UPROPERTY(EditDefaultsOnly, Category = "J3|Style")
	FLinearColor DimColor = FLinearColor(0.f, 0.f, 0.f, 0.5f);

	UPROPERTY(EditDefaultsOnly, Category = "J3|Style")
	TSubclassOf<UJ3ItemSlotWidget> SlotWidgetClass;


	// IJ3ItemSlotHandler
	virtual void HandleSlotClicked(EJ3SlotKind Kind, int32 Index) override;
	virtual void HandleSlotDropped(EJ3SlotKind FromKind, int32 FromIndex, EJ3SlotKind ToKind, int32 ToIndex) override;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UFUNCTION()
	void Refresh();

	UJ3InventoryComponent* GetInventory() const;
	UJ3ItemSlotWidget* MakeSlot(EJ3SlotKind Kind, int32 Index, const FText& CornerLabel);
	void RebuildBagSlots(int32 SlotCount);

	UPROPERTY()
	TObjectPtr<UVerticalBox> EquipmentBox;

	UPROPERTY()
	TObjectPtr<UUniformGridPanel> BagGrid;

	UPROPERTY()
	TArray<TObjectPtr<UJ3ItemSlotWidget>> EquipmentSlots;

	UPROPERTY()
	TArray<TObjectPtr<UJ3ItemSlotWidget>> BagSlots;
};
