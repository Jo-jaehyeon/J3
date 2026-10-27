// Copyright (c) ACCENT. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class APlayerController;
class UWorld;


// 2인 동시 건설·철거 검증 (Roadmap 단계 4 완료 기준: "2인이 동시에 짓고 부수며 결과가 일치한다").
// j3.Debug.AutoBuild N: 접속한 모든 플레이어가 같은 자리 N곳에 동시에 토대를 짓고, 짝수 번째를 동시에 부순다.
// 끝나면 서버·클라이언트가 각자 가진 건물 목록의 개수와 해시를 로그에 남긴다 (콘솔 j3.Build.Dump로도 볼 수 있다).
namespace J3BuildVerify
{
	int32 GetAutoBuildCount();

	// 로컬 플레이어 컨트롤러가 BeginPlay에서 부른다
	void StartAutoBuild(APlayerController* Controller);

	// j3.Debug.AutoFactory: 서버가 가장 가까운 철광석 광맥에 [채굴기 v1 → 컨베이어 → 제련 시설 → 컨베이어 → 창고]와 발전소·전기줄,
	// 배터리로만 도는 제련 시설을 직접 세우고, 창고 주괴 수와 기계 상태를 로그에 남긴다 (단계 5 완료 기준 검증, 입력 없이)
	void StartAutoFactory(UWorld* World);

	// 클라이언트: 같은 시각에 이 PC가 보는 결과(창고 개수, 기계 상태, 벨트 위 아이템)를 남겨 서버와 비교한다
	void StartAutoFactoryClientLog(APlayerController* Controller);

	// 이 월드(서버 또는 클라이언트)의 건물 개수·해시를 로그에 남긴다
	void LogBuildState(UWorld* World, const TCHAR* Reason);
}
