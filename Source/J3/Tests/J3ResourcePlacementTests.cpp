// Copyright (c) ACCENT. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "World/J3ResourcePlacement.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace J3ResourcePlacementTests
{
	FJ3ResourceSpawnDef MakeRule(FName NodeId, int32 Count, int32 Guarantee)
	{
		FJ3ResourceSpawnDef Rule;
		Rule.NodeId = NodeId;
		Rule.Count = Count;
		Rule.Guarantee = Guarantee;
		return Rule;
	}


	// 100m × 100m 구역, 시작 지점은 한쪽 구석 근처, 보장 반경 20m
	J3ResourcePlacement::FSettings MakeSettings(int32 Seed)
	{
		J3ResourcePlacement::FSettings Settings;
		Settings.Seed = Seed;
		Settings.AreaCenter = FVector2D(0.f, 0.f);
		Settings.AreaHalfExtent = FVector2D(5000.f, 5000.f);
		Settings.GuaranteeCenter = FVector2D(-3000.f, -3000.f);
		Settings.GuaranteeRadius = 2000.f;
		Settings.MinSpacing = 300.f;
		return Settings;
	}


	TArray<FJ3ResourceSpawnDef> MakeRules()
	{
		return { MakeRule("iron_deposit", 6, 2), MakeRule("rock", 20, 4), MakeRule("tree", 10, 0) };
	}
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJ3PlacementDeterministicTest, "J3.ResourcePlacement.SameSeedSameResult", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FJ3PlacementDeterministicTest::RunTest(const FString& Parameters)
{
	using namespace J3ResourcePlacementTests;

	// 같은 시드는 같은 배치, 다른 시드는 다른 배치
	const TArray<J3ResourcePlacement::FPlacedNode> A = J3ResourcePlacement::Place(MakeSettings(42), MakeRules());
	const TArray<J3ResourcePlacement::FPlacedNode> B = J3ResourcePlacement::Place(MakeSettings(42), MakeRules());
	const TArray<J3ResourcePlacement::FPlacedNode> C = J3ResourcePlacement::Place(MakeSettings(7), MakeRules());
	TestEqual(TEXT("개수 같음"), A.Num(), B.Num());
	bool bSame = A.Num() == B.Num();
	for (int32 Index = 0; bSame && Index < A.Num(); ++Index)
	{
		bSame = A[Index].NodeId == B[Index].NodeId && A[Index].Location.Equals(B[Index].Location);
	}
	TestTrue(TEXT("같은 시드 = 같은 배치"), bSame);
	TestFalse(TEXT("다른 시드 = 다른 배치"), C.Num() > 0 && A.Num() > 0 && A[0].Location.Equals(C[0].Location));
	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJ3PlacementGuaranteeTest, "J3.ResourcePlacement.GuaranteeNearStart", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FJ3PlacementGuaranteeTest::RunTest(const FString& Parameters)
{
	using namespace J3ResourcePlacementTests;

	// 여러 시드에서 보장 개수가 항상 시작 지점 반경 안에 있다
	for (int32 Seed = 1; Seed <= 50; ++Seed)
	{
		const J3ResourcePlacement::FSettings Settings = MakeSettings(Seed);
		const TArray<J3ResourcePlacement::FPlacedNode> Placed = J3ResourcePlacement::Place(Settings, MakeRules());
		int32 IronNear = 0;
		int32 RockNear = 0;
		for (const J3ResourcePlacement::FPlacedNode& Node : Placed)
		{
			const bool bNear = FVector2D::Distance(Node.Location, Settings.GuaranteeCenter) <= Settings.GuaranteeRadius;
			IronNear += Node.NodeId == "iron_deposit" && bNear ? 1 : 0;
			RockNear += Node.NodeId == "rock" && bNear ? 1 : 0;
		}
		if (IronNear < 2 || RockNear < 4)
		{
			AddError(FString::Printf(TEXT("시드 %d: 반경 안 철 %d(2 이상), 바위 %d(4 이상)"), Seed, IronNear, RockNear));
		}
	}
	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJ3PlacementSpacingTest, "J3.ResourcePlacement.SpacingAndArea", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FJ3PlacementSpacingTest::RunTest(const FString& Parameters)
{
	using namespace J3ResourcePlacementTests;

	// 모든 노드가 구역 안이고 서로 최소 거리 이상 떨어져 있다
	const J3ResourcePlacement::FSettings Settings = MakeSettings(123);
	const TArray<J3ResourcePlacement::FPlacedNode> Placed = J3ResourcePlacement::Place(Settings, MakeRules());
	TestEqual(TEXT("36개 모두 놓임"), Placed.Num(), 36);
	for (int32 I = 0; I < Placed.Num(); ++I)
	{
		const FVector2D Offset = (Placed[I].Location - Settings.AreaCenter).GetAbs();
		TestTrue(TEXT("구역 안"), Offset.X <= Settings.AreaHalfExtent.X && Offset.Y <= Settings.AreaHalfExtent.Y);
		for (int32 J = I + 1; J < Placed.Num(); ++J)
		{
			TestTrue(TEXT("최소 거리"), FVector2D::Distance(Placed[I].Location, Placed[J].Location) >= Settings.MinSpacing - 0.01f);
		}
	}
	return true;
}

#endif
