// Copyright (c) ACCENT. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Building/J3BuildingBase.h"
#include "J3PowerBuildings.generated.h"


// 전신주 (Power_System 2절: 전력을 더 먼 거리까지). 전력망의 노드만 맡는다. 위쪽에 전기 연결점.
UCLASS()
class J3_API AJ3PowerPoleBuilding : public AJ3PowerBuilding
{
	GENERATED_BODY()

public:
	AJ3PowerPoleBuilding();

	virtual bool IsPowerNode() const override
	{
		return true;
	}

	virtual int32 GetPowerNodeId() const override
	{
		return PowerNodeId;
	}

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, Category = "J3|Building")
	TObjectPtr<UJ3PortComponent> PowerPort;

	// 서버: 전력망 노드 번호
	int32 PowerNodeId = INDEX_NONE;
};


// 전기줄 (Power_System 2절). 두 전력 노드(발전소·전신주·전기를 쓰는 기계)를 직접 잇는다.
// 서버는 전력망에 간선을 더하고, 모양은 두 연결점 사이의 가는 원기둥. 이어진 건물을 철거하면 같이 철거되어 환급된다.
UCLASS()
class J3_API AJ3PowerWireBuilding : public AJ3PowerBuilding
{
	GENERATED_BODY()

public:
	AJ3PowerWireBuilding();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 서버: 생성 직후 두 끝을 정한다
	void SetEnds(AJ3BuildingBase* InNodeA, AJ3BuildingBase* InNodeB, const FVector& InPointA, const FVector& InPointB);

	bool Connects(const AJ3BuildingBase* Building) const
	{
		return Building != nullptr && (NodeA == Building || NodeB == Building);
	}

	bool ConnectsPair(const AJ3BuildingBase* A, const AJ3BuildingBase* B) const
	{
		return (NodeA == A && NodeB == B) || (NodeA == B && NodeB == A);
	}

	virtual bool CanSupportBuildingsOnTop() const override
	{
		return false;
	}

	// 이 건물에 이어진 전기줄 (철거할 때 같이 철거, 이미 이어졌는지 판정)
	static TArray<AJ3PowerWireBuilding*> FindWiresOf(const AJ3BuildingBase* Building);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void OnRep_Ends();

	void UpdateShape();

	UPROPERTY(ReplicatedUsing = OnRep_Ends)
	TObjectPtr<AJ3BuildingBase> NodeA;

	UPROPERTY(ReplicatedUsing = OnRep_Ends)
	TObjectPtr<AJ3BuildingBase> NodeB;

	UPROPERTY(ReplicatedUsing = OnRep_Ends)
	FVector_NetQuantize10 PointA;

	UPROPERTY(ReplicatedUsing = OnRep_Ends)
	FVector_NetQuantize10 PointB;

	// 줄 굵기 (기본 원기둥 100cm 기준 배율)
	UPROPERTY(EditDefaultsOnly, Category = "J3|Style")
	float Thickness = 0.06f;

	// 서버: 전력망 간선 번호
	int32 PowerEdgeId = INDEX_NONE;
};
