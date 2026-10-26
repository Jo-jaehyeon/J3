// Copyright (c) ACCENT. All Rights Reserved.

#include "Factory/J3ConveyorPath.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJ3ConveyorStraightTest, "J3.Conveyor.StraightLength", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FJ3ConveyorStraightTest::RunTest(const FString& Parameters)
{
	// 직선은 두 점 사이 거리 그대로, 꺾임이 없다
	FJ3ConveyorPath Path;
	Path.End = FVector(1000.f, 0.f, 0.f);
	TestTrue(TEXT("길이 10m"), FMath::IsNearlyEqual(Path.ComputeLength(), 1000.f, 1.f));
	TestTrue(TEXT("반지름 매우 큼"), Path.ComputeMinRadius() > 100000.f);
	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJ3ConveyorCurveTest, "J3.Conveyor.CurveLength", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FJ3ConveyorCurveTest::RunTest(const FString& Parameters)
{
	// 90° 꺾이는 곡선: 길이는 직선 거리보다 길고(곡선 길이를 실제로 잰다) 반지름 10m 호 길이(약 15.7m)에 가깝다
	FJ3ConveyorPath Path;
	Path.End = FVector(1000.f, 1000.f, 0.f);
	Path.StartDirection = FVector::ForwardVector;
	Path.EndDirection = FVector::RightVector;
	const float Length = Path.ComputeLength();
	TestTrue(TEXT("직선 거리보다 길다"), Length > 1414.f);
	TestTrue(TEXT("호 길이와 10% 안"), FMath::Abs(Length - 1571.f) < 157.f);
	TestTrue(TEXT("반지름 3m 이상"), Path.ComputeMinRadius() > 300.f);


	// 길이표 끝은 끝점
	TArray<FVector> Points;
	TArray<float> Distances;
	Path.BuildArcTable(50, Points, Distances);
	FVector Location;
	FVector Direction;
	FJ3ConveyorPath::SampleArcTable(Points, Distances, Distances.Last(), Location, Direction);
	TestTrue(TEXT("끝점"), Location.Equals(Path.End, 1.f));
	TestTrue(TEXT("끝 방향 +Y"), Direction.Equals(FVector::RightVector, 0.05f));
	return true;
}

#endif
