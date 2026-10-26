// Copyright (c) ACCENT. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/PlayerState.h"
#include "J3PlayerState.generated.h"

class UAbilitySystemComponent;
class UJ3InventoryComponent;
class UJ3SurvivalAttributeSet;
class UJ3SurvivalComponent;


// 플레이어 개인 상태. GAS(능력·속성)를 폰이 아니라 여기에 둬서 사망·부활로 폰이 바뀌어도 수치와 능력이 유지된다.
UCLASS()
class J3_API AJ3PlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AJ3PlayerState();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UJ3SurvivalAttributeSet* GetSurvivalAttributes() const
	{
		return SurvivalAttributes;
	}

	UJ3InventoryComponent* GetInventory() const
	{
		return Inventory;
	}

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "J3|Abilities")
	TObjectPtr<UAbilitySystemComponent> AbilitySystem;

	UPROPERTY()
	TObjectPtr<UJ3SurvivalAttributeSet> SurvivalAttributes;

	UPROPERTY(VisibleAnywhere, Category = "J3|Survival")
	TObjectPtr<UJ3SurvivalComponent> Survival;

	UPROPERTY(VisibleAnywhere, Category = "J3|Inventory")
	TObjectPtr<UJ3InventoryComponent> Inventory;
};
