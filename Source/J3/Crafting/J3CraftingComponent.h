// Copyright (c) ACCENT. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "J3CraftingComponent.generated.h"

struct FJ3RecipeDef;


// 플레이어의 제작 요청을 서버에서 판정한다 (PlayerState에 붙는다).
// 확인: 제작 장소 가까이 있는지, 그 장소에서 만드는 레시피인지, 팀이 해방했는지, 재료가 있는지, 결과물이 들어가는지.
// TODO(기획-미결정): 제작 시간(time_sec)과 연속 제작(Space). 지금은 즉시 1개씩 만든다.
UCLASS(ClassGroup = (J3))
class J3_API UJ3CraftingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UJ3CraftingComponent();

	UFUNCTION(Server, Reliable)
	void Server_Craft(AActor* Station, FName RecipeId);

	// 이 장소에서 지금 만들 수 있는 레시피인지 (재료 제외: 장소·해방만, 화면 목록용)
	bool IsRecipeAvailableAt(const FJ3RecipeDef& Recipe, const AActor* Station) const;

	// 제작 장소와 가까운지 (상호작용 거리 + 여유)
	bool IsNearStation(const AActor* Station) const;
};
