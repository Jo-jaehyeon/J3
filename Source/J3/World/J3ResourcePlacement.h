// Copyright (c) ACCENT. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/J3DataTypes.h"


// 시드 기반 자원 노드 배치 (Resources_And_Tiers 1절): 같은 시드·같은 규칙이면 항상 같은 배치가 나온다.
// 규칙마다 보장 개수를 시작 지점 반경 안에 먼저 놓고, 나머지를 구역 전체에 흩는다. 노드끼리는 최소 거리를 둔다.
// 높이(지면)는 다루지 않는다. 평면 위치만 정하고, 지면에 붙이는 일은 배치 액터가 한다.
namespace J3ResourcePlacement
{
	struct FSettings
	{
		int32 Seed = 0;

		// 구역 (축 정렬 사각형, 중심과 반 크기)
		FVector2D AreaCenter = FVector2D::ZeroVector;
		FVector2D AreaHalfExtent = FVector2D::ZeroVector;

		// 보장 반경 (시작 지점 기준)
		FVector2D GuaranteeCenter = FVector2D::ZeroVector;
		float GuaranteeRadius = 0.f;

		float MinSpacing = 0.f;
		int32 MaxAttempts = 32;
	};

	struct FPlacedNode
	{
		FName NodeId;
		FVector2D Location = FVector2D::ZeroVector;
		bool bGuaranteed = false;
	};

	// 놓지 못한 노드(자리가 부족)는 결과에서 빠진다
	J3_API TArray<FPlacedNode> Place(const FSettings& Settings, const TArray<FJ3ResourceSpawnDef>& Rules);
}
