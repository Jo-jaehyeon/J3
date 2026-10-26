// Copyright (c) ACCENT. All Rights Reserved.

#include "Inventory/J3InventoryLogic.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace J3InventoryLogicTests
{
	// 테스트용 스택 한도: 도구는 1, 나머지는 5
	int32 TestStackSize(FName ItemId)
	{
		return ItemId == "stone_axe" ? 1 : 5;
	}


	FJ3ItemStack MakeStack(FName ItemId, int32 Count)
	{
		FJ3ItemStack Stack;
		Stack.ItemId = ItemId;
		Stack.Count = Count;
		return Stack;
	}


	TArray<FJ3ItemStack> MakeSlots(int32 Num)
	{
		TArray<FJ3ItemStack> Slots;
		Slots.SetNum(Num);
		return Slots;
	}
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJ3InventoryAddTest, "J3.Inventory.Add", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FJ3InventoryAddTest::RunTest(const FString& Parameters)
{
	using namespace J3InventoryLogicTests;

	// 같은 칸부터 채우고 넘치면 다음 빈 칸
	TArray<FJ3ItemStack> Slots = MakeSlots(3);
	TestEqual(TEXT("7개 넣기 남음"), J3InventoryLogic::Add(Slots, "stone", 7, TestStackSize), 0);
	TestEqual(TEXT("0번 칸 5개"), Slots[0].Count, 5);
	TestEqual(TEXT("1번 칸 2개"), Slots[1].Count, 2);


	// 기존 칸을 먼저 채운다
	TestEqual(TEXT("3개 더 넣기 남음"), J3InventoryLogic::Add(Slots, "stone", 3, TestStackSize), 0);
	TestEqual(TEXT("1번 칸 5개"), Slots[1].Count, 5);
	TestTrue(TEXT("2번 칸 비어 있음"), Slots[2].IsEmpty());


	// 가득 차면 남은 개수를 돌려준다
	TestEqual(TEXT("도끼 2개 중 1개 남음"), J3InventoryLogic::Add(Slots, "stone_axe", 2, TestStackSize), 1);
	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJ3InventoryRemoveTest, "J3.Inventory.Remove", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FJ3InventoryRemoveTest::RunTest(const FString& Parameters)
{
	using namespace J3InventoryLogicTests;

	TArray<FJ3ItemStack> Slots = MakeSlots(3);
	J3InventoryLogic::Add(Slots, "arrow", 7, TestStackSize);


	// 모자라면 아무것도 빼지 않는다
	TestFalse(TEXT("8개 빼기 실패"), J3InventoryLogic::Remove(Slots, "arrow", 8));
	TestEqual(TEXT("그대로 7개"), J3InventoryLogic::CountOf(Slots, "arrow"), 7);


	// 뒤쪽 칸부터 뺀다
	TestTrue(TEXT("3개 빼기"), J3InventoryLogic::Remove(Slots, "arrow", 3));
	TestEqual(TEXT("0번 칸 4개"), Slots[0].Count, 4);
	TestTrue(TEXT("1번 칸 비움"), Slots[1].IsEmpty());
	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJ3InventoryMoveTest, "J3.Inventory.Move", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FJ3InventoryMoveTest::RunTest(const FString& Parameters)
{
	using namespace J3InventoryLogicTests;

	TArray<FJ3ItemStack> Slots = MakeSlots(3);
	Slots[0] = MakeStack("stone", 4);
	Slots[1] = MakeStack("stone", 3);
	Slots[2] = MakeStack("branch", 1);


	// 같은 아이템은 한도까지 합친다
	TestTrue(TEXT("합치기"), J3InventoryLogic::Move(Slots, 1, 0, TestStackSize));
	TestEqual(TEXT("0번 칸 5개"), Slots[0].Count, 5);
	TestEqual(TEXT("1번 칸 2개 남음"), Slots[1].Count, 2);


	// 다른 아이템은 자리 바꾸기
	TestTrue(TEXT("자리 바꾸기"), J3InventoryLogic::Move(Slots, 2, 0, TestStackSize));
	TestEqual(TEXT("0번 칸 나뭇가지"), Slots[0].ItemId, FName("branch"));
	TestEqual(TEXT("2번 칸 돌 5개"), Slots[2].Count, 5);


	// 잘못된 번호는 거부
	TestFalse(TEXT("범위 밖"), J3InventoryLogic::Move(Slots, 0, 9, TestStackSize));
	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJ3InventoryCycleTest, "J3.Inventory.CycleEquipment", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FJ3InventoryCycleTest::RunTest(const FString& Parameters)
{
	using namespace J3InventoryLogicTests;

	// 장비: [도끼, 빈 칸, 활, 빈 칸]
	TArray<FJ3ItemStack> Equipment = MakeSlots(4);
	Equipment[0] = MakeStack("stone_axe", 1);
	Equipment[2] = MakeStack("bow", 1);


	// 다음: 맨손 → 0 → 2 → 맨손 (빈 칸 건너뜀)
	TestEqual(TEXT("맨손 다음 0"), J3InventoryLogic::NextActiveIndex(Equipment, J3BareHandIndex, 1), 0);
	TestEqual(TEXT("0 다음 2"), J3InventoryLogic::NextActiveIndex(Equipment, 0, 1), 2);
	TestEqual(TEXT("2 다음 맨손"), J3InventoryLogic::NextActiveIndex(Equipment, 2, 1), J3BareHandIndex);


	// 이전: 맨손 → 2 → 0
	TestEqual(TEXT("맨손 이전 2"), J3InventoryLogic::NextActiveIndex(Equipment, J3BareHandIndex, -1), 2);
	TestEqual(TEXT("2 이전 0"), J3InventoryLogic::NextActiveIndex(Equipment, 2, -1), 0);


	// 장비가 없으면 맨손 유지
	TArray<FJ3ItemStack> Empty = MakeSlots(4);
	TestEqual(TEXT("빈 장비는 맨손"), J3InventoryLogic::NextActiveIndex(Empty, J3BareHandIndex, 1), J3BareHandIndex);
	return true;
}

#endif
