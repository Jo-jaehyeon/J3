// Copyright (c) ACCENT. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"


// 컨베이어 한 구간의 모양 (Building_Placement 3절: 직선 + 곡선). 시작·끝 위치와 방향으로 정하는 3차 에르미트 곡선.
// 로직은 길이만 쓰고(아이템 간격·이동 시간이 길이에 비례), 모양은 표시와 설치 판정에 쓴다.
struct J3_API FJ3ConveyorPath
{
	FVector Start = FVector::ZeroVector;
	FVector End = FVector::ZeroVector;

	// 시작·끝에서 진행 방향 (단위 벡터). 끝 방향은 끝점에 들어가는 방향
	FVector StartDirection = FVector::ForwardVector;
	FVector EndDirection = FVector::ForwardVector;

	// 곡선 위 점·접선 (T: 0~1, 길이에 비례하지 않는다)
	FVector Evaluate(float T) const;
	FVector Derivative(float T) const;

	// 실제 곡선 길이 (잘게 나눠 더한다)
	float ComputeLength() const;

	// 가장 급하게 꺾이는 곳의 반지름 (직선이면 매우 큼)
	float ComputeMinRadius() const;

	// 길이를 따라 고르게 놓인 점 (표시: 아이템 위치를 오프셋 → 위치로 바꿀 때)
	void BuildArcTable(int32 SampleCount, TArray<FVector>& OutPoints, TArray<float>& OutDistances) const;

	// 길이표에서 시작으로부터 Distance만큼 간 위치와 방향
	static void SampleArcTable(const TArray<FVector>& Points, const TArray<float>& Distances, float Distance, FVector& OutLocation, FVector& OutDirection);
};
