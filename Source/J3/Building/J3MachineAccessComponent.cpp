// Copyright (c) ACCENT. All Rights Reserved.

#include "Building/J3MachineAccessComponent.h"

#include "Building/J3MachineBuildings.h"
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


bool UJ3MachineAccessComponent::IsNearMachine(const AJ3MachineBuilding* Machine) const
{
	const AJ3PlayerState* PlayerState = Cast<AJ3PlayerState>(GetOwner());
	const AJ3Character* Character = PlayerState != nullptr ? PlayerState->GetPawn<AJ3Character>() : nullptr;
	if (Character == nullptr || Machine == nullptr)
	{
		return false;
	}
	const UJ3DataSubsystem* Data = UJ3DataSubsystem::Get();
	const float AllowedCm = Data->GetConfigFloat("interact_range_cm", 100.f) + Data->GetConfigFloat("interact_tolerance_cm", 0.f);
	return Character->GetBodyDistanceTo(Machine) <= AllowedCm;
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
		[Data](FName Item) { const FJ3FuelDef* Fuel = Data->GetFuel(Item); return Fuel != nullptr ? Fuel->BurnSec : 0.f; });
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
