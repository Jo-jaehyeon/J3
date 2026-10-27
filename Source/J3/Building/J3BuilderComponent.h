// Copyright (c) ACCENT. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "J3BuilderComponent.generated.h"

class AJ3BuildingBase;


// 플레이어의 설치·철거 요청을 서버에서 처리한다 (PlayerState에 붙는다, Building_Placement 4절).
// 설치: 서버가 같은 판정을 다시 하고, 통과하면 재료를 빼고 건물을 만든다.
// 철거: 재료를 전부 돌려준다 (100% 환급). 가방이 차서 못 넣은 재료는 그 자리에 떨군다.
UCLASS(ClassGroup = (J3))
class J3_API UJ3BuilderComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UJ3BuilderComponent();

	UFUNCTION(Server, Reliable)
	void Server_PlaceBuilding(FName BuildingId, FVector Location, float YawDeg, bool bGrid);

	// 컨베이어: 시작점·끝점 조준과 바닥 끝 방향 (서버가 같은 판정으로 곡선을 정한다)
	UFUNCTION(Server, Reliable)
	void Server_PlaceConveyor(FName BuildingId, FVector StartAim, FVector EndAim, float EndYawOffsetDeg);

	// 전기줄: 두 전력 노드 조준
	UFUNCTION(Server, Reliable)
	void Server_PlaceWire(FName BuildingId, FVector AimA, FVector AimB);

	UFUNCTION(Server, Reliable)
	void Server_Demolish(AJ3BuildingBase* Building);

	// 이 건물을 지금 철거할 수 있는지 (거리)
	bool CanDemolish(const AJ3BuildingBase* Building) const;

private:
	class AJ3Character* GetBuilder() const;
	void DropLeftover(FName ItemId, int32 Count, const FVector& Location) const;

	// 건설 재료 + 안에 든 아이템을 돌려주고 없앤다
	void RefundAndDestroy(AJ3BuildingBase* Building) const;
};
