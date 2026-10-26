// Copyright (c) ACCENT. All Rights Reserved.

#include "Inventory/J3InventoryLogic.h"


namespace J3InventoryLogic
{
	int32 Add(TArray<FJ3ItemStack>& Slots, FName ItemId, int32 Count, FStackSizeFn StackSize)
	{
		if (ItemId.IsNone() || Count <= 0)
		{
			return Count;
		}
		const int32 MaxStack = FMath::Max(1, StackSize(ItemId));
		int32 Remaining = Count;


		// 이미 같은 아이템이 있는 칸부터 채운다
		for (FJ3ItemStack& Slot : Slots)
		{
			if (Remaining == 0)
			{
				break;
			}
			if (Slot.ItemId == ItemId && Slot.Count < MaxStack)
			{
				const int32 Moved = FMath::Min(Remaining, MaxStack - Slot.Count);
				Slot.Count += Moved;
				Remaining -= Moved;
			}
		}


		// 남은 것은 앞쪽 빈 칸부터
		for (FJ3ItemStack& Slot : Slots)
		{
			if (Remaining == 0)
			{
				break;
			}
			if (Slot.IsEmpty())
			{
				const int32 Moved = FMath::Min(Remaining, MaxStack);
				Slot.ItemId = ItemId;
				Slot.Count = Moved;
				Remaining -= Moved;
			}
		}
		return Remaining;
	}


	int32 CountOf(const TArray<FJ3ItemStack>& Slots, FName ItemId)
	{
		int32 Total = 0;
		for (const FJ3ItemStack& Slot : Slots)
		{
			if (Slot.ItemId == ItemId)
			{
				Total += Slot.Count;
			}
		}
		return Total;
	}


	bool Remove(TArray<FJ3ItemStack>& Slots, FName ItemId, int32 Count)
	{
		if (Count <= 0 || CountOf(Slots, ItemId) < Count)
		{
			return false;
		}


		// 뒤쪽 칸부터 빼서 앞쪽(자주 보는 칸)의 묶음을 남긴다
		int32 Remaining = Count;
		for (int32 Index = Slots.Num() - 1; Index >= 0 && Remaining > 0; --Index)
		{
			FJ3ItemStack& Slot = Slots[Index];
			if (Slot.ItemId != ItemId)
			{
				continue;
			}
			const int32 Taken = FMath::Min(Remaining, Slot.Count);
			Slot.Count -= Taken;
			Remaining -= Taken;
			if (Slot.Count == 0)
			{
				Slot.Clear();
			}
		}
		return true;
	}


	int32 FindFirstEmpty(const TArray<FJ3ItemStack>& Slots)
	{
		return Slots.IndexOfByPredicate([](const FJ3ItemStack& Slot)
		{
			return Slot.IsEmpty();
		});
	}


	bool Move(TArray<FJ3ItemStack>& Slots, int32 From, int32 To, FStackSizeFn StackSize)
	{
		if (!Slots.IsValidIndex(From) || !Slots.IsValidIndex(To) || From == To || Slots[From].IsEmpty())
		{
			return false;
		}
		FJ3ItemStack& Source = Slots[From];
		FJ3ItemStack& Target = Slots[To];


		// 같은 아이템: 한도까지 합친다
		if (Target.ItemId == Source.ItemId)
		{
			const int32 MaxStack = FMath::Max(1, StackSize(Source.ItemId));
			const int32 Moved = FMath::Min(Source.Count, MaxStack - Target.Count);
			if (Moved <= 0)
			{
				return false;
			}
			Target.Count += Moved;
			Source.Count -= Moved;
			if (Source.Count == 0)
			{
				Source.Clear();
			}
			return true;
		}


		// 다른 아이템 또는 빈 칸: 자리 바꾸기
		Swap(Source, Target);
		return true;
	}


	bool HasAll(const TArray<FJ3ItemStack>& Slots, const TArray<FJ3ItemAmount>& Inputs)
	{
		// 같은 재료가 두 줄로 적혀 있을 수 있으니 합쳐서 센다
		TMap<FName, int32> Needed;
		for (const FJ3ItemAmount& Input : Inputs)
		{
			Needed.FindOrAdd(Input.ItemId) += Input.Count;
		}
		for (const TPair<FName, int32>& Pair : Needed)
		{
			if (CountOf(Slots, Pair.Key) < Pair.Value)
			{
				return false;
			}
		}
		return true;
	}


	bool TryExchange(TArray<FJ3ItemStack>& Slots, const TArray<FJ3ItemAmount>& Inputs, const FJ3ItemAmount& Output, FStackSizeFn StackSize)
	{
		// 사본에서 해 보고 성공했을 때만 반영한다 (재료를 뺀 자리에 결과물이 들어갈 수도 있다)
		if (!HasAll(Slots, Inputs))
		{
			return false;
		}
		TArray<FJ3ItemStack> Working = Slots;
		for (const FJ3ItemAmount& Input : Inputs)
		{
			Remove(Working, Input.ItemId, Input.Count);
		}
		if (Add(Working, Output.ItemId, Output.Count, StackSize) > 0)
		{
			return false;
		}
		Slots = MoveTemp(Working);
		return true;
	}


	int32 NextActiveIndex(const TArray<FJ3ItemStack>& Equipment, int32 CurrentIndex, int32 Direction)
	{
		// 순환 순서: 맨손(-1), 0, 1, ..., N-1. 맨손은 항상 고를 수 있다.
		// TODO(기획-제안): 맨손 포함·빈 칸 건너뜀은 Inventory_And_UI 3절의 제안값
		const int32 Step = Direction >= 0 ? 1 : -1;
		const int32 PositionCount = Equipment.Num() + 1;
		int32 Position = CurrentIndex + 1;
		for (int32 Tried = 0; Tried < PositionCount; ++Tried)
		{
			Position = (Position + Step + PositionCount) % PositionCount;
			const int32 Candidate = Position - 1;
			if (Candidate == J3BareHandIndex || !Equipment[Candidate].IsEmpty())
			{
				return Candidate;
			}
		}
		return J3BareHandIndex;
	}
}
