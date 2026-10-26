// Copyright (c) ACCENT. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "J3CraftingStationComponent.generated.h"


// 이 액터가 제작 장소임을 나타낸다. 레시피의 station 열과 같은 이름을 갖는다 (ai_module, base_workbench ...).
// 비행기 AI 모듈과 기지(작업대 포함, 단계 6)가 이 컴포넌트를 붙여 같은 제작 흐름을 쓴다.
UCLASS(ClassGroup = (J3), meta = (BlueprintSpawnableComponent))
class J3_API UJ3CraftingStationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UJ3CraftingStationComponent();

	FName GetStationId() const
	{
		return StationId;
	}

protected:
	UPROPERTY(EditAnywhere, Category = "J3|Crafting")
	FName StationId;
};
