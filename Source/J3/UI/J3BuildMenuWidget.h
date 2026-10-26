// Copyright (c) ACCENT. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "J3BuildMenuWidget.generated.h"

class UBorder;
class UHorizontalBox;
class UJ3BuildMenuWidget;
class UTextBlock;
class UVerticalBox;


// 건설 메뉴의 누를 수 있는 칸 (탭 또는 건물 줄)
UCLASS()
class J3_API UJ3BuildMenuEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Setup(UJ3BuildMenuWidget* InOwner, FName InKey, bool bInIsTab);
	void Refresh(const FText& Label, const FText& Detail, const FLinearColor& Color);

protected:
	virtual void NativeOnInitialized() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

private:
	UPROPERTY()
	TObjectPtr<UBorder> Background;

	UPROPERTY()
	TObjectPtr<UTextBlock> LabelText;

	UPROPERTY()
	TObjectPtr<UTextBlock> DetailText;

	TWeakObjectPtr<UJ3BuildMenuWidget> Owner;
	FName Key;
	bool bIsTab = false;
};


// 건설 메뉴 (Q, Inventory_And_UI 1.3): 탭(생산·물류·전력·구조물·기지)별로 해방된 건물만 보여 준다.
// 각 건물은 필요 재료와 보유량을 보여 주고 부족하면 빨간색이다. 클릭하면 건설 모드로 들어간다.
// 건물에 마우스를 올리고 숫자키를 누르면 단축 칸에 등록한다. Q·Esc로 닫는다.
// TODO(기획-제안): 검색 칸
UCLASS()
class J3_API UJ3BuildMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void HandleEntryClicked(FName Key, bool bIsTab);
	void HandleEntryHovered(FName Key, bool bIsTab, bool bHovered);

	UPROPERTY(EditDefaultsOnly, Category = "J3|Style")
	FLinearColor DimColor = FLinearColor(0.f, 0.f, 0.f, 0.5f);

	UPROPERTY(EditDefaultsOnly, Category = "J3|Style")
	FLinearColor TabColor = FLinearColor(0.12f, 0.12f, 0.12f, 0.85f);

	UPROPERTY(EditDefaultsOnly, Category = "J3|Style")
	FLinearColor SelectedTabColor = FLinearColor(0.95f, 0.75f, 0.2f, 0.9f);

	UPROPERTY(EditDefaultsOnly, Category = "J3|Style")
	FLinearColor ReadyColor = FLinearColor(0.15f, 0.3f, 0.15f, 0.85f);

	UPROPERTY(EditDefaultsOnly, Category = "J3|Style")
	FLinearColor MissingColor = FLinearColor(0.3f, 0.12f, 0.12f, 0.85f);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	void Refresh();
	UJ3BuildMenuEntryWidget* MakeEntry(FName Key, bool bIsTab);

	UPROPERTY()
	TObjectPtr<UHorizontalBox> TabRow;

	UPROPERTY()
	TObjectPtr<UVerticalBox> BuildingList;

	UPROPERTY()
	TMap<FName, TObjectPtr<UJ3BuildMenuEntryWidget>> Tabs;

	UPROPERTY()
	TMap<FName, TObjectPtr<UJ3BuildMenuEntryWidget>> Rows;

	FName SelectedTab = "production";
	FName HoveredBuilding;
};
