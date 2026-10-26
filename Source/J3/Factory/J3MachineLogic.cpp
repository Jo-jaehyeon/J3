// Copyright (c) ACCENT. All Rights Reserved.

#include "Factory/J3MachineLogic.h"

#include "Data/J3DataTypes.h"


namespace J3MachineLogicPrivate
{
	FJ3ItemStack& SlotOf(FJ3MachineState& State, EJ3MachineSlot Slot)
	{
		return Slot == EJ3MachineSlot::Input ? State.Input : (Slot == EJ3MachineSlot::Fuel ? State.Fuel : State.Output);
	}


	// 출력 칸에 같은 아이템으로 Count개를 더 쌓을 수 있는지
	bool HasOutputRoom(const FJ3MachineState& State, const FJ3MachineDef& Def, FName ItemId, int32 Count)
	{
		return (State.Output.IsEmpty() || State.Output.ItemId == ItemId) && State.Output.Count + Count <= Def.OutputCap;
	}


	void AddToOutput(FJ3MachineState& State, FName ItemId, int32 Count)
	{
		State.Output.ItemId = ItemId;
		State.Output.Count += Count;
	}


	// 주기 진행. 다 차면 true
	bool Advance(FJ3MachineState& State, float CycleSec, float DeltaSec)
	{
		State.CycleSec = CycleSec;
		State.ProgressSec += DeltaSec;
		if (State.ProgressSec < CycleSec)
		{
			return false;
		}
		State.ProgressSec -= CycleSec;
		return true;
	}


	EJ3MachineStatus StepMiner(FJ3MachineState& State, const FJ3MachineDef& Def, float DeltaSec, bool& bOutChanged)
	{
		// 캘 자원이 없거나 출력이 차면 멈춘다 (진행도는 남겨 둔다)
		if (State.ResourceItem.IsNone())
		{
			return EJ3MachineStatus::NoResource;
		}
		if (!HasOutputRoom(State, Def, State.ResourceItem, State.ResourceYield))
		{
			return EJ3MachineStatus::OutputFull;
		}
		if (Advance(State, Def.CycleSec, DeltaSec))
		{
			AddToOutput(State, State.ResourceItem, State.ResourceYield);
			bOutChanged = true;
		}
		return EJ3MachineStatus::Running;
	}


	EJ3MachineStatus StepSmelter(FJ3MachineState& State, const FJ3MachineDef& Def, float DeltaSec, J3MachineLogic::FFindRecipe FindRecipe, J3MachineLogic::FFuelBurnSec FuelBurnSec, bool& bOutChanged)
	{
		// 입력 칸의 재료로 만들 레시피와 재료가 충분한지
		const FJ3RecipeDef* Recipe = State.Input.IsEmpty() ? nullptr : FindRecipe(State.Input.ItemId);
		const FJ3ItemAmount* Needed = Recipe != nullptr ? Recipe->Inputs.FindByPredicate([&State](const FJ3ItemAmount& Amount) { return Amount.ItemId == State.Input.ItemId; }) : nullptr;
		if (Recipe == nullptr || Needed == nullptr || State.Input.Count < Needed->Count)
		{
			State.ProgressSec = 0.f;
			return EJ3MachineStatus::NoInput;
		}
		if (!HasOutputRoom(State, Def, Recipe->Output.ItemId, Recipe->Output.Count))
		{
			return EJ3MachineStatus::OutputFull;
		}


		// 연료 칸이 있는 기계는 탈 연료가 있어야 돈다 (가동 중에만 탄다)
		if (Def.FuelCap > 0 && State.FuelLeftSec <= 0.f)
		{
			const float BurnSec = State.Fuel.IsEmpty() ? 0.f : FuelBurnSec(State.Fuel.ItemId);
			if (BurnSec <= 0.f)
			{
				return EJ3MachineStatus::NoFuel;
			}
			State.FuelLeftSec += BurnSec;
			--State.Fuel.Count;
			if (State.Fuel.Count <= 0)
			{
				State.Fuel.Clear();
			}
			bOutChanged = true;
		}
		if (Def.FuelCap > 0)
		{
			State.FuelLeftSec -= DeltaSec;
		}


		// 주기가 끝나면 재료를 쓰고 결과를 쌓는다
		const float CycleSec = Recipe->TimeSec > 0.f ? Recipe->TimeSec : Def.CycleSec;
		if (Advance(State, CycleSec, DeltaSec))
		{
			State.Input.Count -= Needed->Count;
			if (State.Input.Count <= 0)
			{
				State.Input.Clear();
			}
			AddToOutput(State, Recipe->Output.ItemId, Recipe->Output.Count);
			bOutChanged = true;
		}
		return EJ3MachineStatus::Running;
	}
}


namespace J3MachineLogic
{
	bool Step(FJ3MachineState& State, const FJ3MachineDef& Def, float DeltaSec, FFindRecipe FindRecipe, FFuelBurnSec FuelBurnSec)
	{
		using namespace J3MachineLogicPrivate;

		bool bChanged = false;
		const EJ3MachineStatus Status = Def.IsMiner() ? StepMiner(State, Def, DeltaSec, bChanged) : StepSmelter(State, Def, DeltaSec, FindRecipe, FuelBurnSec, bChanged);
		bChanged |= Status != State.Status;
		State.Status = Status;
		return bChanged;
	}


	int32 Insert(FJ3MachineState& State, const FJ3MachineDef& Def, FName ItemId, int32 Count, FFindRecipe FindRecipe, FFuelBurnSec FuelBurnSec)
	{
		using namespace J3MachineLogicPrivate;

		// 연료가 먼저 (나뭇가지처럼 재료이자 연료인 것은 연료 칸으로), 그다음 레시피 재료
		EJ3MachineSlot Slot = EJ3MachineSlot::Input;
		int32 Cap = 0;
		if (Def.FuelCap > 0 && FuelBurnSec(ItemId) > 0.f)
		{
			Slot = EJ3MachineSlot::Fuel;
			Cap = Def.FuelCap;
		}
		else if (Def.InputCap > 0 && FindRecipe(ItemId) != nullptr)
		{
			Slot = EJ3MachineSlot::Input;
			Cap = Def.InputCap;
		}
		else
		{
			return 0;
		}


		// 칸이 비었거나 같은 아이템일 때 상한까지
		FJ3ItemStack& Target = SlotOf(State, Slot);
		if (!Target.IsEmpty() && Target.ItemId != ItemId)
		{
			return 0;
		}
		const int32 Accepted = FMath::Clamp(Cap - Target.Count, 0, Count);
		if (Accepted > 0)
		{
			Target.ItemId = ItemId;
			Target.Count += Accepted;
		}
		return Accepted;
	}


	FJ3ItemStack Take(FJ3MachineState& State, EJ3MachineSlot Slot, int32 MaxCount)
	{
		using namespace J3MachineLogicPrivate;

		FJ3ItemStack& Source = SlotOf(State, Slot);
		FJ3ItemStack Taken;
		Taken.ItemId = Source.ItemId;
		Taken.Count = FMath::Clamp(MaxCount, 0, Source.Count);
		Source.Count -= Taken.Count;
		if (Source.Count <= 0)
		{
			Source.Clear();
		}
		return Taken;
	}
}
