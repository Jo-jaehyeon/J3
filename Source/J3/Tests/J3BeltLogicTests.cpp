// Copyright (c) ACCENT. All Rights Reserved.

#include "Factory/J3BeltLogic.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace J3BeltLogicTests
{
	// 길이 100cm, 초당 100cm, 간격 20cm 구간들을 Id 순서대로 만들고 차례로 잇는다
	struct FTestBelt
	{
		TArray<FJ3BeltSegment> Segments;
		TMap<int32, int32> IndexById;
		TSet<int32> Changed;

		explicit FTestBelt(int32 SegmentCount, bool bLoop = false)
		{
			for (int32 Index = 0; Index < SegmentCount; ++Index)
			{
				FJ3BeltSegment Segment;
				Segment.Id = Index;
				Segment.LengthCm = 100.f;
				Segment.SpeedCmPerSec = 100.f;
				Segment.SpacingCm = 20.f;
				const bool bHasNext = Index + 1 < SegmentCount || bLoop;
				Segment.NextId = bHasNext ? (Index + 1) % SegmentCount : INDEX_NONE;
				Segments.Add(Segment);
				IndexById.Add(Index, Index);
			}
		}

		void Step(float DeltaSec)
		{
			J3BeltLogic::Step(Segments, IndexById, DeltaSec, Changed);
		}
	};
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJ3BeltMoveTest, "J3.Belt.Move", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FJ3BeltMoveTest::RunTest(const FString& Parameters)
{
	using namespace J3BeltLogicTests;

	// 0.3초에 30cm 이동
	FTestBelt Belt(1);
	TestTrue(TEXT("넣기"), J3BeltLogic::TryInsert(Belt.Segments[0], 1));
	Belt.Step(0.3f);
	TestEqual(TEXT("30cm"), Belt.Segments[0].Items[0].Offset, 30.f, 0.01f);


	// 다음 구간이 없으면 끝에서 멈춘다
	Belt.Step(5.f);
	TestEqual(TEXT("끝에서 멈춤"), Belt.Segments[0].Items[0].Offset, 100.f, 0.01f);
	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJ3BeltSpacingTest, "J3.Belt.Spacing", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FJ3BeltSpacingTest::RunTest(const FString& Parameters)
{
	using namespace J3BeltLogicTests;

	// 시작에 자리가 없으면 넣을 수 없다
	FJ3BeltSegment Segment;
	Segment.LengthCm = 100.f;
	Segment.SpeedCmPerSec = 100.f;
	Segment.SpacingCm = 20.f;
	TestTrue(TEXT("첫 아이템"), J3BeltLogic::TryInsert(Segment, 1));
	TestFalse(TEXT("바로 뒤는 자리 없음"), J3BeltLogic::TryInsert(Segment, 1));


	// 끝에 막히면 뒤 아이템은 간격을 두고 쌓인다
	FTestBelt Belt(1);
	for (int32 Count = 0; Count < 3; ++Count)
	{
		J3BeltLogic::TryInsert(Belt.Segments[0], 1);
		Belt.Step(0.25f);
	}
	Belt.Step(5.f);
	const TArray<FJ3BeltItem>& Items = Belt.Segments[0].Items;
	TestEqual(TEXT("3개"), Items.Num(), 3);
	TestEqual(TEXT("앞 100"), Items[0].Offset, 100.f, 0.01f);
	TestEqual(TEXT("둘째 80"), Items[1].Offset, 80.f, 0.01f);
	TestEqual(TEXT("셋째 60"), Items[2].Offset, 60.f, 0.01f);
	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJ3BeltTransferTest, "J3.Belt.Transfer", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FJ3BeltTransferTest::RunTest(const FString& Parameters)
{
	using namespace J3BeltLogicTests;

	// 90cm에서 0.3초(30cm) → 다음 구간 20cm 지점 (경계에서 거리 손실·중복 이동 없음)
	FTestBelt Belt(2);
	J3BeltLogic::TryInsert(Belt.Segments[0], 7);
	Belt.Segments[0].Items[0].Offset = 90.f;
	Belt.Step(0.3f);
	TestEqual(TEXT("앞 구간 비움"), Belt.Segments[0].Items.Num(), 0);
	TestEqual(TEXT("다음 구간 1개"), Belt.Segments[1].Items.Num(), 1);
	TestEqual(TEXT("20cm 지점"), Belt.Segments[1].Items[0].Offset, 20.f, 0.01f);
	TestEqual(TEXT("종류 유지"), static_cast<int32>(Belt.Segments[1].Items[0].Type), 7);
	TestTrue(TEXT("두 구간 변경 이벤트"), Belt.Changed.Contains(0) && Belt.Changed.Contains(1));
	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJ3BeltBlockedTest, "J3.Belt.Blocked", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FJ3BeltBlockedTest::RunTest(const FString& Parameters)
{
	using namespace J3BeltLogicTests;

	// 다음 구간 시작이 막혀 있으면 끝에서 기다린다
	FTestBelt Belt(2);
	J3BeltLogic::TryInsert(Belt.Segments[1], 1);
	Belt.Segments[1].Items[0].Offset = 5.f;
	Belt.Segments[1].SpeedCmPerSec = 0.f;
	J3BeltLogic::TryInsert(Belt.Segments[0], 2);
	Belt.Step(2.f);
	TestEqual(TEXT("앞 구간에 남음"), Belt.Segments[0].Items.Num(), 1);
	TestEqual(TEXT("끝에서 대기"), Belt.Segments[0].Items[0].Offset, 100.f, 0.01f);
	TestEqual(TEXT("다음 구간 그대로"), Belt.Segments[1].Items.Num(), 1);
	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJ3BeltLoopTest, "J3.Belt.LoopConservesItems", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FJ3BeltLoopTest::RunTest(const FString& Parameters)
{
	using namespace J3BeltLogicTests;

	// 닫힌 고리에서 오래 돌려도 아이템 수가 그대로이고 간격이 지켜진다
	FTestBelt Belt(4, true);
	for (FJ3BeltSegment& Segment : Belt.Segments)
	{
		for (int32 Count = 0; Count < 3; ++Count)
		{
			FJ3BeltItem Item;
			Item.Type = 1;
			Item.Offset = 80.f - Count * 30.f;
			Segment.Items.Add(Item);
		}
	}
	for (int32 Tick = 0; Tick < 2000; ++Tick)
	{
		Belt.Step(1.f / 20.f);
	}
	TestEqual(TEXT("12개 유지"), J3BeltLogic::CountItems(Belt.Segments), 12);
	for (const FJ3BeltSegment& Segment : Belt.Segments)
	{
		for (int32 Index = 1; Index < Segment.Items.Num(); ++Index)
		{
			TestTrue(TEXT("간격 유지"), Segment.Items[Index - 1].Offset - Segment.Items[Index].Offset >= Segment.SpacingCm - 0.01f);
		}
	}
	return true;
}

#endif
