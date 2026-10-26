// Copyright (c) ACCENT. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "J3CraftingWidget.generated.h"

class UBorder;
class UJ3CraftingWidget;
class UTextBlock;
class UVerticalBox;


// 제작 목록의 한 줄: [결과물] [재료 보유/필요]. 클릭하면 제작 요청.
UCLASS()
class J3_API UJ3RecipeRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Setup(UJ3CraftingWidget* InOwner, FName InRecipeId);
	void Refresh(bool bCanCraft, const FText& OutputText, const FText& InputsText);

	UPROPERTY(EditDefaultsOnly, Category = "J3|Style")
	FLinearColor ReadyColor = FLinearColor(0.15f, 0.3f, 0.15f, 0.85f);

	UPROPERTY(EditDefaultsOnly, Category = "J3|Style")
	FLinearColor MissingColor = FLinearColor(0.3f, 0.12f, 0.12f, 0.85f);

protected:
	virtual void NativeOnInitialized() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

private:
	UPROPERTY()
	TObjectPtr<UBorder> Background;

	UPROPERTY()
	TObjectPtr<UTextBlock> OutputLabel;

	UPROPERTY()
	TObjectPtr<UTextBlock> InputsLabel;

	TWeakObjectPtr<UJ3CraftingWidget> Owner;
	FName RecipeId;
};


// 제작 화면 (Tutorial 2단계: AI 모듈에 말을 걸면 제작 가능한 목록이 뜬다).
// 이 장소에서 만들 수 있고 팀이 해방한 레시피만 보여 주고, 재료가 모자라면 빨간 줄로 표시한다.
// Esc로 닫고, 장소에서 멀어지면 저절로 닫힌다. 위젯 구성은 코드, 겉모습은 블루프린트에서 조정한다.
// TODO(기획-미결정): 제작 UI 구성(Inventory_And_UI 3절), 연속 제작(Space)
UCLASS()
class J3_API UJ3CraftingWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetStation(AActor* InStation);

	AActor* GetStation() const
	{
		return Station.Get();
	}

	// 줄을 클릭했을 때
	void RequestCraft(FName RecipeId);

	UPROPERTY(EditDefaultsOnly, Category = "J3|Style")
	FLinearColor DimColor = FLinearColor(0.f, 0.f, 0.f, 0.5f);

	UPROPERTY(EditDefaultsOnly, Category = "J3|Style")
	TSubclassOf<UJ3RecipeRowWidget> RowWidgetClass;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	// 목록 다시 그리기 (해방·재료가 바뀌면 줄 색과 수가 바뀐다)
	void Refresh();

	UPROPERTY()
	TObjectPtr<UVerticalBox> RecipeList;

	UPROPERTY()
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY()
	TMap<FName, TObjectPtr<UJ3RecipeRowWidget>> Rows;

	TWeakObjectPtr<AActor> Station;
};
