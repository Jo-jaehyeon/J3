// Copyright (c) ACCENT. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Building/J3BuildingBase.h"
#include "Factory/J3ConveyorPath.h"
#include "J3ConveyorBuilding.generated.h"

class UBoxComponent;


// 복제용 컨베이어 모양 (시작·끝 위치와 방향)
USTRUCT()
struct FJ3ConveyorPathNet
{
	GENERATED_BODY()

	UPROPERTY()
	FVector_NetQuantize10 Start;

	UPROPERTY()
	FVector_NetQuantize10 End;

	UPROPERTY()
	FVector_NetQuantizeNormal StartDirection;

	UPROPERTY()
	FVector_NetQuantizeNormal EndDirection;

	FJ3ConveyorPath ToPath() const;
	static FJ3ConveyorPathNet FromPath(const FJ3ConveyorPath& Path);
};


// 컨베이어 한 구간 (Coding_Rules 3.1 ALogisticsBuilding, Building_Placement 3절). 포트(또는 다른 컨베이어 끝)에서 포트로 잇는 곡선.
// 서버는 곡선 길이로 공장 시뮬레이션 구간을 만들고, 앞뒤 컨베이어와 잇는다. 벨트와 아이템은 공장 표시(AJ3BeltVisualizer)가 그린다.
// 이 Actor는 철거 조준을 위한 충돌 상자와 설치·철거 진입점만 맡는다.
UCLASS()
class J3_API AJ3ConveyorBuilding : public AJ3LogisticsBuilding
{
	GENERATED_BODY()

public:
	AJ3ConveyorBuilding();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 서버: 생성 직후 모양을 정한다
	void SetPath(const FJ3ConveyorPath& InPath);

	FJ3ConveyorPath GetPath() const
	{
		return PathNet.ToPath();
	}

	int32 GetSegmentId() const
	{
		return SegmentId;
	}

	// 벨트 위 아이템 (철거하면 돌려준다)
	virtual void GetStoredItems(TArray<FJ3ItemStack>& OutItems) const override;

	// 서버: 시작점·끝점에 맞닿은 컨베이어·기계·창고와 잇는다 (나중에 지어진 쪽에서도 다시 부른다)
	void LinkNeighbors();

	// 서버: 이 건물의 포트에 닿을 수 있는 컨베이어를 모두 다시 잇는다 (기계·창고가 컨베이어보다 나중에 지어졌을 때)
	static void RelinkConveyorsNear(const AJ3BuildingBase* Building);

	// 컨베이어 위에는 짓지 않는다
	virtual bool CanSupportBuildingsOnTop() const override
	{
		return false;
	}

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void OnRep_PathNet();

	// 곡선을 따라 충돌 상자를 놓는다 (조준·겹침 판정용, 플레이어는 막지 않는다)
	void RebuildCollision();

	UPROPERTY(ReplicatedUsing = OnRep_PathNet)
	FJ3ConveyorPathNet PathNet;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UBoxComponent>> CollisionBoxes;

	// 서버: 공장 시뮬레이션 구간 번호
	int32 SegmentId = INDEX_NONE;
};
