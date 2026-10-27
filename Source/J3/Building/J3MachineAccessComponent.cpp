// Copyright (c) ACCENT. All Rights Reserved.

#include "Building/J3MachineAccessComponent.h"

#include "Building/J3MachineBuildings.h"
#include "Building/J3StorageBuilding.h"
#include "Core/J3PlayerController.h"
#include "Inventory/J3InventoryLogic.h"
#include "Core/J3PlayerState.h"
#include "Data/J3DataSubsystem.h"
#include "Engine/World.h"
#include "Factory/J3FactorySimSubsystem.h"
#include "Factory/J3MachineLogic.h"
#include "Inventory/J3InventoryComponent.h"
#include "Player/J3Character.h"


UJ3MachineAccessComponent::UJ3MachineAccessComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}


bool UJ3MachineAccessComponent::IsNearBuilding(const AActor* Building) const
{
	const AJ3PlayerState* PlayerState = Cast<AJ3PlayerState>(GetOwner());
	const AJ3Character* Character = PlayerState != nullptr ? PlayerState->GetPawn<AJ3Character>() : nullptr;
	if (Character == nullptr || Building == nullptr)
	{
		return false;
	}
	const UJ3DataSubsystem* Data = UJ3DataSubsystem::Get();
	const float AllowedCm = Data->GetConfigFloat("interact_range_cm", 100.f) + Data->GetConfigFloat("interact_tolerance_cm", 0.f);
	return Character->GetBodyDistanceTo(Building) <= AllowedCm;
}


bool UJ3MachineAccessComponent::IsNearMachine(const AJ3MachineBuilding* Machine) const
{
	return IsNearBuilding(Machine);
}


void UJ3MachineAccessComponent::Server_StoreFromInventory_Implementation(AJ3StorageBuilding* Storage, int32 InventorySlot)
{
	// 가방 칸을 통째로 넣고, 창고가 차서 못 넣은 만큼은 가방에 남긴다
	UJ3InventoryComponent* Inventory = Cast<AJ3PlayerState>(GetOwner())->GetInventory();
	if (!IsNearBuilding(Storage) || !Inventory->GetSlots().IsValidIndex(InventorySlot) || Inventory->GetSlots()[InventorySlot].IsEmpty())
	{
		return;
	}
	const FJ3ItemStack Stack = Inventory->GetSlots()[InventorySlot];
	const int32 Leftover = Storage->AddItem(Stack.ItemId, Stack.Count);
	if (Leftover < Stack.Count)
	{
		Inventory->RemoveItem(Stack.ItemId, Stack.Count - Leftover);
	}
}


void UJ3MachineAccessComponent::Server_TakeFromStorage_Implementation(AJ3StorageBuilding* Storage, int32 StorageSlot)
{
	// 창고 칸을 통째로 꺼내고, 가방이 차서 못 넣은 만큼은 창고에 남긴다
	UJ3InventoryComponent* Inventory = Cast<AJ3PlayerState>(GetOwner())->GetInventory();
	TArray<FJ3ItemStack>* Slots = Storage != nullptr ? &Storage->GetSlotsMutable() : nullptr;
	if (!IsNearBuilding(Storage) || Slots == nullptr || !Slots->IsValidIndex(StorageSlot) || (*Slots)[StorageSlot].IsEmpty())
	{
		return;
	}
	FJ3ItemStack& Stack = (*Slots)[StorageSlot];
	const int32 Leftover = Inventory->AddItem(Stack.ItemId, Stack.Count);
	Stack.Count = Leftover;
	if (Stack.Count <= 0)
	{
		Stack.Clear();
	}
	Storage->NotifyChanged();
}


void UJ3MachineAccessComponent::Server_MoveStorageSlot_Implementation(AJ3StorageBuilding* Storage, int32 FromSlot, int32 ToSlot)
{
	const UJ3DataSubsystem* Data = UJ3DataSubsystem::Get();
	if (IsNearBuilding(Storage) && J3InventoryLogic::Move(Storage->GetSlotsMutable(), FromSlot, ToSlot, [Data](FName Item) { return Data->GetStackSize(Item); }))
	{
		Storage->NotifyChanged();
	}
}


void UJ3MachineAccessComponent::Server_CloseStorage_Implementation(AJ3StorageBuilding* Storage)
{
	const AJ3PlayerState* PlayerState = Cast<AJ3PlayerState>(GetOwner());
	if (Storage != nullptr && PlayerState != nullptr)
	{
		Storage->RemoveViewer(Cast<AJ3PlayerController>(PlayerState->GetOwner()));
	}
}


void UJ3MachineAccessComponent::Server_InsertFromInventory_Implementation(AJ3MachineBuilding* Machine, int32 InventorySlot)
{
	// 거리, 가방 칸, 기계 상태 확인
	UJ3InventoryComponent* Inventory = Cast<AJ3PlayerState>(GetOwner())->GetInventory();
	UJ3FactorySimSubsystem* Sim = GetWorld()->GetSubsystem<UJ3FactorySimSubsystem>();
	FJ3MachineState* State = Sim != nullptr && Machine != nullptr ? Sim->FindMachineMutable(Machine->GetMachineId()) : nullptr;
	const FJ3MachineDef* Def = Machine != nullptr ? Machine->GetMachineDef() : nullptr;
	if (!IsNearMachine(Machine) || State == nullptr || Def == nullptr || !Inventory->GetSlots().IsValidIndex(InventorySlot))
	{
		return;
	}
	const FJ3ItemStack Stack = Inventory->GetSlots()[InventorySlot];
	if (Stack.IsEmpty())
	{
		return;
	}


	// 기계에 들어간 만큼 가방에서 뺀다
	const UJ3DataSubsystem* Data = UJ3DataSubsystem::Get();
	const FName BuildingId = Machine->GetBuildingId();
	const int32 Accepted = J3MachineLogic::Insert(*State, *Def, Stack.ItemId, Stack.Count,
		[BuildingId](FName Item) { return UJ3FactorySimSubsystem::FindMachineRecipe(BuildingId, Item); },
		[Data](FName Item) { return Data->GetFuelBurnSec(Item); },
		[Data](FName Item) { return Data->GetBatteryEnergy(Item); });
	if (Accepted > 0)
	{
		Inventory->RemoveItem(Stack.ItemId, Accepted);
		Sim->NotifyMachineChanged(State->Id);
	}
}


void UJ3MachineAccessComponent::Server_TakeToInventory_Implementation(AJ3MachineBuilding* Machine, EJ3MachineSlot Slot)
{
	UJ3InventoryComponent* Inventory = Cast<AJ3PlayerState>(GetOwner())->GetInventory();
	UJ3FactorySimSubsystem* Sim = GetWorld()->GetSubsystem<UJ3FactorySimSubsystem>();
	FJ3MachineState* State = Sim != nullptr && Machine != nullptr ? Sim->FindMachineMutable(Machine->GetMachineId()) : nullptr;
	if (!IsNearMachine(Machine) || State == nullptr)
	{
		return;
	}


	// 다 꺼내 가방에 넣고, 못 넣은 만큼은 기계 칸에 되돌린다
	FJ3ItemStack Taken = J3MachineLogic::Take(*State, Slot, TNumericLimits<int32>::Max());
	if (Taken.IsEmpty())
	{
		return;
	}
	const int32 Leftover = Inventory->AddItem(Taken.ItemId, Taken.Count);
	if (Leftover > 0)
	{
		FJ3ItemStack& Source = Slot == EJ3MachineSlot::Input ? State->Input : (Slot == EJ3MachineSlot::Fuel ? State->Fuel : State->Output);
		Source.ItemId = Taken.ItemId;
		Source.Count += Leftover;
	}
	Sim->NotifyMachineChanged(State->Id);
}
