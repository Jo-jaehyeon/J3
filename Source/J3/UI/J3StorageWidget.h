// Copyright (c) ACCENT. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/J3ItemSlotWidget.h"
#include "J3StorageWidget.generated.h"

class AJ3StorageBuilding;
class UTextBlock;
class UUniformGridPanel;


// 창고 화면 (G키). 위에 창고 칸, 아래에 내 가방. 창고 내용은 서버가 보내 준 것을 그린다.
// 가방 칸 클릭(또는 창고로 끌기) → 창고에 넣기, 창고 칸 클릭(또는 가방으로 끌기) → 꺼내기, 창고 안에서 끌기 → 옮기기.
// Esc로 닫고, 멀어지면 저절로 닫힌다. 위젯 구성은 코드, 겉모습은 블루프린트에서 조정한다.
UCLASS()
class J3_API UJ3StorageWidget : public UUserWidget, public IJ3ItemSlotHandler
{
	GENERATED_BODY()

public:
	void SetStorage(AJ3StorageBuilding* InStorage, const TArray<FJ3ItemStack>& InSlots);

	AJ3StorageBuilding* GetStorage() const
	{
		return Storage.Get();
	}

	virtual void HandleSlotClicked(EJ3SlotKind Kind, int32 Index) override;
	virtual void HandleSlotDropped(EJ3SlotKind FromKind, int32 FromIndex, EJ3SlotKind ToKind, int32 ToIndex) override;

	UPROPERTY(EditDefaultsOnly, Category = "J3|Style")
	FLinearColor DimColor = FLinearColor(0.f, 0.f, 0.f, 0.5f);

	UPROPERTY(EditDefaultsOnly, Category = "J3|Style")
	TSubclassOf<UJ3ItemSlotWidget> SlotWidgetClass;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	UJ3ItemSlotWidget* MakeSlot(EJ3SlotKind Kind, int32 Index);
	void RefreshGrid(UUniformGridPanel* Grid, TArray<TObjectPtr<UJ3ItemSlotWidget>>& Widgets, EJ3SlotKind Kind, const TArray<FJ3ItemStack>& Items);

	UPROPERTY()
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY()
	TObjectPtr<UUniformGridPanel> StorageGrid;

	UPROPERTY()
	TObjectPtr<UUniformGridPanel> BagGrid;

	UPROPERTY()
	TArray<TObjectPtr<UJ3ItemSlotWidget>> StorageSlots;

	UPROPERTY()
	TArray<TObjectPtr<UJ3ItemSlotWidget>> BagSlots;

	TWeakObjectPtr<AJ3StorageBuilding> Storage;
	TArray<FJ3ItemStack> StorageItems;
};
