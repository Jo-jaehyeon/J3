// Copyright (c) ACCENT. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UJ3FactorySimSubsystem;


// 단계 2 시뮬레이션·복제 측정용 벤치마크 (개발 전용).
// 정사각형 컨베이어 고리(구간 4개)를 격자로 깔고 아이템을 채워 계속 돌린다.
//   콘솔: j3.Bench.Belts <구간 수> <구간당 아이템 수>
//   시작 설정: j3.Bench.AutoSpawn "구간 수x구간당 아이템 수" (예: 400x10. 명령줄 -ini 값에는 쉼표를 쓸 수 없다)
namespace J3FactoryBench
{
	void SpawnRings(UJ3FactorySimSubsystem& Simulation, int32 SegmentCount, int32 ItemsPerSegment);
	void SpawnFromConsoleVariable(UJ3FactorySimSubsystem& Simulation);

	// 클라이언트: 벤치마크 중이면 받은 상태를 같은 형식으로 기록한다 (서버와 아이템 수 비교)
	void StartClientStatsFromConsoleVariable(UJ3FactorySimSubsystem& Simulation);
}
