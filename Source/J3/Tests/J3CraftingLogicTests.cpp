// Copyright (c) ACCENT. All Rights Reserved.

#include "Inventory/J3InventoryLogic.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace J3CraftingLogicTests
{
	int32 TestStackSize(FName ItemId)
	{
		return ItemId == "stone_pickaxe" ? 1 : 5;
	}


	FJ3ItemAmount Amount(FName ItemId, int32 Count)
	{
		FJ3ItemAmount Result;
		Result.ItemId = ItemId;
		Result.Count = Count;
		return Result;
	}


	TArray<FJ3ItemStack> MakeSlots(int32 Num)
	{
		TArray<FJ3ItemStack> Slots;
		Slots.SetNum(Num);
		return Slots;
	}
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJ3CraftSuccessTest, "J3.Crafting.StonePickaxe", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FJ3CraftSuccessTest::RunTest(const FString& Parameters)
{
	using namespace J3CraftingLogicTests;

	// 돌 1 + 나뭇가지 1 → 돌곡괭이 1 (단계 3 완료 기준)
	TArray<FJ3ItemStack> Slots = MakeSlots(3);
	J3InventoryLogic::Add(Slots, "stone", 2, TestStackSize);
	J3InventoryLogic::Add(Slots, "branch", 1, TestStackSize);
	const TArray<FJ3ItemAmount> Inputs = { Amount("stone", 1), Amount("branch", 1) };
	TestTrue(TEXT("제작 성공"), J3InventoryLogic::TryExchange(Slots, Inputs, Amount("stone_pickaxe", 1), TestStackSize));
	TestEqual(TEXT("돌 1 남음"), J3InventoryLogic::CountOf(Slots, "stone"), 1);
	TestEqual(TEXT("나뭇가지 없음"), J3InventoryLogic::CountOf(Slots, "branch"), 0);
	TestEqual(TEXT("곡괭이 1"), J3InventoryLogic::CountOf(Slots, "stone_pickaxe"), 1);
	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJ3CraftMissingTest, "J3.Crafting.MissingInputChangesNothing", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FJ3CraftMissingTest::RunTest(const FString& Parameters)
{
	using namespace J3CraftingLogicTests;

	// 재료가 모자라면 아무것도 빠지지 않는다
	TArray<FJ3ItemStack> Slots = MakeSlots(3);
	J3InventoryLogic::Add(Slots, "stone", 1, TestStackSize);
	const TArray<FJ3ItemStack> Before = Slots;
	TestFalse(TEXT("실패"), J3InventoryLogic::TryExchange(Slots, { Amount("stone", 1), Amount("branch", 1) }, Amount("stone_pickaxe", 1), TestStackSize));
	TestTrue(TEXT("그대로"), Slots == Before);


	// 같은 재료가 두 줄이면 합쳐서 확인한다
	TestFalse(TEXT("돌 2개 필요"), J3InventoryLogic::HasAll(Slots, { Amount("stone", 1), Amount("stone", 1) }));
	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJ3CraftNoRoomTest, "J3.Crafting.NoRoomChangesNothing", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FJ3CraftNoRoomTest::RunTest(const FString& Parameters)
{
	using namespace J3CraftingLogicTests;

	// 결과물이 들어갈 자리가 없으면 재료도 빠지지 않는다
	TArray<FJ3ItemStack> Slots = MakeSlots(2);
	J3InventoryLogic::Add(Slots, "stone", 5, TestStackSize);
	J3InventoryLogic::Add(Slots, "branch", 5, TestStackSize);
	const TArray<FJ3ItemStack> Before = Slots;
	TestFalse(TEXT("자리 없음"), J3InventoryLogic::TryExchange(Slots, { Amount("stone", 1), Amount("branch", 1) }, Amount("stone_pickaxe", 1), TestStackSize));
	TestTrue(TEXT("그대로"), Slots == Before);


	// 재료를 빼서 칸이 비면 그 자리에 들어간다
	TArray<FJ3ItemStack> Tight = MakeSlots(2);
	J3InventoryLogic::Add(Tight, "stone", 1, TestStackSize);
	J3InventoryLogic::Add(Tight, "branch", 1, TestStackSize);
	TestTrue(TEXT("빈 칸에 들어감"), J3InventoryLogic::TryExchange(Tight, { Amount("stone", 1), Amount("branch", 1) }, Amount("stone_pickaxe", 1), TestStackSize));
	return true;
}

#endif
