// Copyright (c) ACCENT. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Factory/J3BeltTypes.h"


// 컨베이어 이동 규칙만 담은 순수 함수 모음. 서버와 클라이언트가 같은 코드로 같은 결과를 내도록 하고, 자동 테스트로 검증한다.
namespace J3BeltLogic
{
	// 구간 시작에 아이템을 놓을 자리가 있는지 (맨 뒤 아이템이 간격 이상 떨어져 있어야 한다)
	J3_API bool HasRoomAtStart(const FJ3BeltSegment& Segment);

	// 구간 시작에 아이템을 놓는다. 자리가 없으면 false.
	J3_API bool TryInsert(FJ3BeltSegment& Segment, FJ3ItemTypeId Type);

	// 모든 구간을 DeltaSec만큼 움직인다. 구간 순서(Id 오름차순)가 같으면 결과도 같다.
	// 아이템이 들어오거나 나간 구간 Id를 OutChangedSegments에 모은다 (복제 이벤트용).
	J3_API void Step(TArray<FJ3BeltSegment>& Segments, const TMap<int32, int32>& IndexById, float DeltaSec, TSet<int32>& OutChangedSegments);

	J3_API int32 CountItems(const TArray<FJ3BeltSegment>& Segments);
}
