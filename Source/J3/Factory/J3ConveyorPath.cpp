// Copyright (c) ACCENT. All Rights Reserved.

#include "Factory/J3ConveyorPath.h"

#include "Algo/BinarySearch.h"


namespace J3ConveyorPathPrivate
{
	// 길이·곡률 계산 분할 수 (구간 하나에 이 정도면 길이 오차가 1% 미만)
	constexpr int32 MeasureSteps = 64;


	// 에르미트 접선 크기: 두 점 사이 거리 (자연스러운 S자·호 모양)
	float TangentScale(const FJ3ConveyorPath& Path)
	{
		return FVector::Dist(Path.Start, Path.End);
	}
}


FVector FJ3ConveyorPath::Evaluate(float T) const
{
	const float Scale = J3ConveyorPathPrivate::TangentScale(*this);
	return FMath::CubicInterp(Start, StartDirection * Scale, End, EndDirection * Scale, T);
}


FVector FJ3ConveyorPath::Derivative(float T) const
{
	const float Scale = J3ConveyorPathPrivate::TangentScale(*this);
	return FMath::CubicInterpDerivative(Start, StartDirection * Scale, End, EndDirection * Scale, T);
}


float FJ3ConveyorPath::ComputeLength() const
{
	using namespace J3ConveyorPathPrivate;

	float Length = 0.f;
	FVector Previous = Start;
	for (int32 Step = 1; Step <= MeasureSteps; ++Step)
	{
		const FVector Point = Evaluate(static_cast<float>(Step) / MeasureSteps);
		Length += FVector::Dist(Previous, Point);
		Previous = Point;
	}
	return Length;
}


float FJ3ConveyorPath::ComputeMinRadius() const
{
	using namespace J3ConveyorPathPrivate;

	// 이웃한 짧은 조각 사이의 꺾인 각도로 반지름을 어림한다 (반지름 = 조각 길이 / 각도)
	float MinRadius = TNumericLimits<float>::Max();
	FVector PreviousDirection = Derivative(0.f).GetSafeNormal();
	FVector PreviousPoint = Start;
	for (int32 Step = 1; Step <= MeasureSteps; ++Step)
	{
		const float T = static_cast<float>(Step) / MeasureSteps;
		const FVector Point = Evaluate(T);
		const FVector Direction = Derivative(T).GetSafeNormal();
		const float Angle = FMath::Acos(FMath::Clamp(FVector::DotProduct(PreviousDirection, Direction), -1.f, 1.f));
		if (Angle > KINDA_SMALL_NUMBER)
		{
			MinRadius = FMath::Min(MinRadius, FVector::Dist(PreviousPoint, Point) / Angle);
		}
		PreviousDirection = Direction;
		PreviousPoint = Point;
	}
	return MinRadius;
}


void FJ3ConveyorPath::BuildArcTable(int32 SampleCount, TArray<FVector>& OutPoints, TArray<float>& OutDistances) const
{
	const int32 Count = FMath::Max(2, SampleCount);
	OutPoints.Reset(Count);
	OutDistances.Reset(Count);
	float Distance = 0.f;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const FVector Point = Evaluate(static_cast<float>(Index) / (Count - 1));
		Distance += Index > 0 ? FVector::Dist(OutPoints.Last(), Point) : 0.f;
		OutPoints.Add(Point);
		OutDistances.Add(Distance);
	}
}


void FJ3ConveyorPath::SampleArcTable(const TArray<FVector>& Points, const TArray<float>& Distances, float Distance, FVector& OutLocation, FVector& OutDirection)
{
	// 거리표에서 앞뒤 두 점 사이를 비율로 잇는다
	if (Points.Num() < 2)
	{
		OutLocation = Points.Num() > 0 ? Points[0] : FVector::ZeroVector;
		OutDirection = FVector::ForwardVector;
		return;
	}
	const int32 Upper = FMath::Clamp(Algo::LowerBound(Distances, Distance), 1, Points.Num() - 1);
	const float Span = FMath::Max(Distances[Upper] - Distances[Upper - 1], KINDA_SMALL_NUMBER);
	const float Alpha = FMath::Clamp((Distance - Distances[Upper - 1]) / Span, 0.f, 1.f);
	OutLocation = FMath::Lerp(Points[Upper - 1], Points[Upper], Alpha);
	OutDirection = (Points[Upper] - Points[Upper - 1]).GetSafeNormal();
}
