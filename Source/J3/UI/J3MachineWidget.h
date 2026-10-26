// Copyright (c) ACCENT. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/J3ItemSlotWidget.h"
#include "J3MachineWidget.generated.h"

class AJ3MachineBuilding;
class UProgressBar;
class UTextBlock;
class UUniformGridPanel;


// 기계 화면 (채굴기·제련로, G키). 위에 기계 칸(입력·연료·출력)과 상태·진행 막대, 아래에 내 가방.
// 가방 칸 클릭(또는 기계 칸으로 끌기) → 알맞은 칸에 넣기, 기계 칸 클릭(또는 가방으로 끌기) → 꺼내기.
// Esc로 닫고, 기계에서 멀어지면 저절로 닫힌다. 위젯 구성은 코드, 겉모습은 블루프린트에서 조정한다.
UCLASS()
class J3_API UJ3MachineWidget : public UUserWidget, public IJ3ItemSlotHandler
{
	GENERATED_BODY()

public:
	void SetMachine(AJ3MachineBuilding* InMachine);

	AJ3MachineBuilding* GetMachine() const
	{
		return Machine.Get();
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
	UJ3ItemSlotWidget* MakeSlot(EJ3SlotKind Kind, int32 Index, const FText& CornerLabel);
	void Refresh();

	UPROPERTY()
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY()
	TObjectPtr<UTextBlock> StatusText;

	UPROPERTY()
	TObjectPtr<UProgressBar> ProgressBar;

	UPROPERTY()
	TObjectPtr<class UHorizontalBox> MachineSlotBox;

	UPROPERTY()
	TObjectPtr<UUniformGridPanel> BagGrid;

	// 기계 칸 (EJ3MachineSlot 순서, 없는 칸은 null)
	UPROPERTY()
	TArray<TObjectPtr<UJ3ItemSlotWidget>> MachineSlots;

	UPROPERTY()
	TArray<TObjectPtr<UJ3ItemSlotWidget>> BagSlots;

	TWeakObjectPtr<AJ3MachineBuilding> Machine;
};
