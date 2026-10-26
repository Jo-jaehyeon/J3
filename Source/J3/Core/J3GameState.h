// Copyright (c) ACCENT. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "J3GameState.generated.h"

struct FJ3UnlockRule;


// 팀(세이브) 공용 상태: 월드 시드(자원 노드 분포), 팀이 한 번이라도 얻은 아이템(해방 판정).
// 해방은 팀 공유다: 누구든 처음 얻으면 모두에게 해방된다 (Progression, Multiplayer_Architecture 4.3).
// TODO(단계 8): 세이브에 저장하고 불러온다
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


	// 서버: 누군가 아이템을 얻었다 (처음이면 기록해 그 아이템으로 해방되는 레시피·건물을 연다)
	void NotifyItemObtained(FName ItemId);

	bool HasObtained(FName ItemId) const
	{
		return ObtainedItems.Contains(ItemId);
	}

	// 해방 조건(auto / on_obtain:아이템)을 만족하는지 (서버·클라이언트 모두)
	bool IsUnlocked(const FJ3UnlockRule& Rule) const;

private:
	UPROPERTY(Replicated)
	int32 WorldSeed = 0;

	// 팀이 한 번이라도 얻은 아이템 (얻은 순서)
	UPROPERTY(Replicated)
	TArray<FName> ObtainedItems;
};
