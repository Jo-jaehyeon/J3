// Copyright (c) ACCENT. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "J3HUDWidget.generated.h"

class UJ3ItemSlotWidget;
class UProgressBar;
class UTextBlock;


// 인게임 HUD (Inventory_And_UI 1.2): 왼쪽 아래 HP·공복·갈증, 오른쪽 아래 장비 4칸(손에 든 칸 강조), 가운데 조준점.
// 위젯 구성은 코드로 만들고, 색·크기는 블루프린트(WBP)에서 바꿀 수 있다.
// TODO(단계 6): 퀘스트 박스, TODO(단계 7): 미니맵
UCLASS()
class J3_API UJ3HUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 겉모습 (블루프린트에서 조정)
	UPROPERTY(EditDefaultsOnly, Category = "J3|Style")
	FLinearColor HealthColor = FLinearColor(0.85f, 0.2f, 0.2f);

	UPROPERTY(EditDefaultsOnly, Category = "J3|Style")
	FLinearColor HungerColor = FLinearColor(0.9f, 0.6f, 0.2f);

	UPROPERTY(EditDefaultsOnly, Category = "J3|Style")
	FLinearColor ThirstColor = FLinearColor(0.25f, 0.55f, 0.95f);

	UPROPERTY(EditDefaultsOnly, Category = "J3|Style")
	FVector2D BarSize = FVector2D(160.f, 12.f);

	UPROPERTY(EditDefaultsOnly, Category = "J3|Style")
	FLinearColor HarvestColor = FLinearColor(0.9f, 0.9f, 0.9f);

	UPROPERTY(EditDefaultsOnly, Category = "J3|Style")
	float ScreenMargin = 24.f;

	UPROPERTY(EditDefaultsOnly, Category = "J3|Style")
	TSubclassOf<UJ3ItemSlotWidget> SlotWidgetClass;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	struct FStatRow
	{
		TObjectPtr<UProgressBar> Bar;
		TObjectPtr<UTextBlock> Value;
	};

	void BuildStats(class UCanvasPanel* Root);
	void BuildEquipment(class UCanvasPanel* Root);
	void BuildCrosshair(class UCanvasPanel* Root);
	void BuildHarvestBar(class UCanvasPanel* Root);
	void BuildBuildModeText(class UCanvasPanel* Root);
	FStatRow AddStatRow(class UVerticalBox* Box, const FText& Label, const FLinearColor& Color);
	static void SetStat(const FStatRow& Row, float Current, float Max);

	// HUD는 값이 몇 개뿐이라 매 프레임 읽어 표시한다 (PlayerState가 늦게 복제돼도 따로 기다릴 필요가 없다)
	FStatRow HealthRow;
	FStatRow HungerRow;
	FStatRow ThirstRow;

	UPROPERTY()
	TArray<TObjectPtr<UJ3ItemSlotWidget>> EquipmentSlots;

	// 채취 진행 막대 (채취 중에만 보임)
	UPROPERTY()
	TObjectPtr<UProgressBar> HarvestBar;

	// 건설 모드 안내: 건물 이름, 설치 가능 여부·불가 사유, 조작 (건설 모드에서만 보임)
	UPROPERTY()
	TObjectPtr<UTextBlock> BuildModeText;
};
