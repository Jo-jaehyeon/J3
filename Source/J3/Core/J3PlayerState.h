// Copyright (c) ACCENT. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/PlayerState.h"
#include "J3PlayerState.generated.h"

class UAbilitySystemComponent;
class UJ3BuilderComponent;
class UJ3MachineAccessComponent;
class UJ3CraftingComponent;
class UJ3EquipmentAbilityComponent;
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

	UJ3CraftingComponent* GetCrafting() const
	{
		return Crafting;
	}

	UJ3BuilderComponent* GetBuilder() const
	{
		return Builder;
	}

	UJ3MachineAccessComponent* GetMachineAccess() const
	{
		return MachineAccess;
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

	UPROPERTY(VisibleAnywhere, Category = "J3|Abilities")
	TObjectPtr<UJ3EquipmentAbilityComponent> EquipmentAbilities;

	UPROPERTY(VisibleAnywhere, Category = "J3|Crafting")
	TObjectPtr<UJ3CraftingComponent> Crafting;

	UPROPERTY(VisibleAnywhere, Category = "J3|Building")
	TObjectPtr<UJ3BuilderComponent> Builder;

	UPROPERTY(VisibleAnywhere, Category = "J3|Building")
	TObjectPtr<UJ3MachineAccessComponent> MachineAccess;
};
