// Copyright (c) ACCENT. All Rights Reserved.

#include "Data/J3DataTypes.h"
#include "Factory/J3MachineLogic.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace J3MachineLogicTests
{
	FJ3MachineDef MakeMiner()
	{
		FJ3MachineDef Def;
		Def.Kind = "miner";
		Def.CycleSec = 1.f;
		Def.OutputCap = 3;
		return Def;
	}


	FJ3MachineDef MakeSmelter()
	{
		FJ3MachineDef Def;
		Def.Kind = "smelter";
		Def.CycleSec = 1.f;
		Def.OutputCap = 10;
		Def.InputCap = 10;
		Def.FuelCap = 10;
		return Def;
	}


	FJ3RecipeDef MakeIngotRecipe()
	{
		FJ3RecipeDef Recipe;
		Recipe.Output.ItemId = "iron_ingot";
		Recipe.Output.Count = 1;
		FJ3ItemAmount Input;
		Input.ItemId = "iron_ore";
		Input.Count = 1;
		Recipe.Inputs.Add(Input);
		return Recipe;
	}


	// 시간을 고정 틱으로 나눠 진행한다 (실제 시뮬레이션과 같은 방식)
	void Run(FJ3MachineState& State, const FJ3MachineDef& Def, float Seconds, const FJ3RecipeDef* Recipe)
	{
		const float StepSec = 0.05f;
		for (float Elapsed = 0.f; Elapsed < Seconds - KINDA_SMALL_NUMBER; Elapsed += StepSec)
		{
			J3MachineLogic::Step(State, Def, StepSec,
				[Recipe](FName Item) { return Recipe != nullptr && Item == "iron_ore" ? Recipe : nullptr; },
				[](FName Item) { return Item == "branch" ? 2.f : 0.f; });
		}
	}
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJ3MinerAccumulateTest, "J3.Machine.MinerStopsAtCap", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FJ3MinerAccumulateTest::RunTest(const FString& Parameters)
{
	using namespace J3MachineLogicTests;

	// 컨베이어가 없으면 상한까지 쌓이고 멈춘다. 꺼내면 다시 돈다.
	const FJ3MachineDef Def = MakeMiner();
	FJ3MachineState State;
	State.ResourceItem = "stone";
	Run(State, Def, 10.f, nullptr);
	TestEqual(TEXT("상한 3개까지 쌓임"), State.Output.Count, 3);
	TestTrue(TEXT("출력 가득으로 멈춤"), State.Status == EJ3MachineStatus::OutputFull);
	const FJ3ItemStack Taken = J3MachineLogic::Take(State, EJ3MachineSlot::Output, 2);
	TestEqual(TEXT("2개 꺼냄"), Taken.Count, 2);
	Run(State, Def, 1.f, nullptr);
	TestEqual(TEXT("다시 채굴"), State.Output.Count, 2);
	TestTrue(TEXT("가동 중"), State.Status == EJ3MachineStatus::Running);
	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJ3SmelterFuelTest, "J3.Machine.SmelterNeedsFuel", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FJ3SmelterFuelTest::RunTest(const FString& Parameters)
{
	using namespace J3MachineLogicTests;

	const FJ3MachineDef Def = MakeSmelter();
	const FJ3RecipeDef Recipe = MakeIngotRecipe();
	FJ3MachineState State;
	auto FindRecipe = [&Recipe](FName Item) { return Item == "iron_ore" ? &Recipe : nullptr; };
	auto FuelBurn = [](FName Item) { return Item == "branch" ? 2.f : 0.f; };


	// 광석은 입력 칸, 나뭇가지는 연료 칸, 연료도 재료도 아닌 것은 못 넣는다
	TestEqual(TEXT("광석 5개"), J3MachineLogic::Insert(State, Def, "iron_ore", 5, FindRecipe, FuelBurn), 5);
	TestEqual(TEXT("돌은 못 넣음"), J3MachineLogic::Insert(State, Def, "stone", 1, FindRecipe, FuelBurn), 0);
	Run(State, Def, 3.f, &Recipe);
	TestTrue(TEXT("연료 없으면 멈춤"), State.Status == EJ3MachineStatus::NoFuel);
	TestEqual(TEXT("주괴 없음"), State.Output.Count, 0);


	// 나뭇가지 1개(2초) = 주괴 2개
	TestEqual(TEXT("나뭇가지 1개"), J3MachineLogic::Insert(State, Def, "branch", 1, FindRecipe, FuelBurn), 1);
	Run(State, Def, 5.f, &Recipe);
	TestEqual(TEXT("주괴 2개"), State.Output.Count, 2);
	TestEqual(TEXT("광석 3개 남음"), State.Input.Count, 3);
	TestTrue(TEXT("연료가 다 타서 멈춤"), State.Status == EJ3MachineStatus::NoFuel);
	return true;
}

#endif
