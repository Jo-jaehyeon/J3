// Copyright (c) ACCENT. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "J3GameState.generated.h"


// 팀(세이브) 공용 상태. 지금은 월드 시드만 둔다 (자원 노드 분포, Multiplayer_Architecture 4.3).
// TODO(단계 8): 세이브에 시드를 저장하고 불러온다
UCLASS()
class J3_API AJ3GameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	int32 GetWorldSeed() const
	{
		return WorldSeed;
	}

	// 서버: 게임 시작 시 GameMode가 정한다
	void SetWorldSeed(int32 InSeed)
	{
		WorldSeed = InSeed;
	}

private:
	UPROPERTY(Replicated)
	int32 WorldSeed = 0;
};
