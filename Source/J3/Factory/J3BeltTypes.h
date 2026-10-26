// Copyright (c) ACCENT. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "J3BeltTypes.generated.h"


// 컨베이어 위 아이템 종류 번호. 0은 "없음". 아이템 ID(FName)와의 대응은 데이터 표 순서로 정한다.
using FJ3ItemTypeId = uint16;


// 컨베이어 위 아이템 하나 (Actor가 아니다, Multiplayer_Architecture 4.2.1)
USTRUCT()
struct FJ3BeltItem
{
	GENERATED_BODY()

	// 구간 시작점에서 얼마나 왔는지 (cm)
	UPROPERTY()
	float Offset = 0.f;

	UPROPERTY()
	uint16 Type = 0;
};


// 컨베이어 구간 하나. 모양(직선·곡선)은 표시용이고 로직은 길이와 다음 구간만 쓴다 (Building_Placement 3절).
struct FJ3BeltSegment
{
	int32 Id = INDEX_NONE;

	// 곡선이면 실제 곡선 길이
	float LengthCm = 0.f;

	float SpeedCmPerSec = 0.f;

	// 아이템 사이 최소 간격 (아이템 크기)
	float SpacingCm = 0.f;

	// 끝에 닿은 아이템이 넘어갈 구간 (없으면 끝에서 멈춘다)
	int32 NextId = INDEX_NONE;

	// 앞(끝에 가까운 것)부터 뒤 순서. [0]이 가장 앞.
	TArray<FJ3BeltItem> Items;
};
