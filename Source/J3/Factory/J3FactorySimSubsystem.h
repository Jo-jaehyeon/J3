// Copyright (c) ACCENT. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Factory/J3BeltTypes.h"
#include "Factory/J3MachineTypes.h"
#include "J3FactorySimSubsystem.generated.h"

class AJ3FactoryReplicator;
class AJ3MachineBuilding;
class AJ3StorageBuilding;
struct FJ3RecipeDef;

DECLARE_LOG_CATEGORY_EXTERN(LogJ3Factory, Log, All);

DECLARE_MULTICAST_DELEGATE_OneParam(FJ3OnSegmentsChanged, const TSet<int32>& /*SegmentIds*/);


// 공장 시뮬레이션 (Multiplayer_Architecture 4.2.1): Actor 밖 데이터 배열을 게임 프레임과 분리된 고정 틱으로 돌린다.
// 건물 Actor는 표시·상호작용만 맡고, 매 틱 계산은 여기서 한다 (Coding_Rules 3.3).
UCLASS()
class J3_API UJ3FactorySimSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;


	// 구간 추가·삭제 (Id 오름차순을 유지해 서버·클라이언트의 처리 순서를 같게 한다)
	void AddSegment(const FJ3BeltSegment& Segment);
	void RemoveSegment(int32 SegmentId);
	void ClearAll();

	bool InsertItem(int32 SegmentId, FJ3ItemTypeId Type);

	// 서버: 새 구간 번호 (컨베이어 건물마다 하나)
	int32 AllocateSegmentId()
	{
		return NextSegmentId++;
	}

	// 서버: 이어지는 다음 구간을 바꾼다 (컨베이어를 잇거나 철거할 때)
	void SetSegmentNext(int32 SegmentId, int32 NextId);

	const FJ3BeltSegment* FindSegment(int32 SegmentId) const;
	FJ3BeltSegment* FindSegmentMutable(int32 SegmentId);

	const TArray<FJ3BeltSegment>& GetSegments() const
	{
		return Segments;
	}

	int32 CountItems() const;

	float GetFixedStepSec() const
	{
		return FixedStepSec;
	}

	// 표시 보간용: 마지막 고정 틱 이후 지난 시간
	float GetTimeSinceStep() const
	{
		return Accumulator;
	}

	// 구간이 추가·삭제될 때마다 오른다 (표시 쪽이 컨베이어 모양을 다시 만들지 판단)
	int32 GetTopologyVersion() const
	{
		return TopologyVersion;
	}

	// 측정: 지난 호출 이후 고정 틱 계산 시간의 평균·최대 (ms)를 돌려주고 초기화한다
	void ConsumeStepStats(double& OutAverageMs, double& OutMaxMs);

	// 아이템이 들어오거나 나간 구간 (복제용)
	FJ3OnSegmentsChanged OnSegmentsChanged;


	// 서버: 기계(채굴기·제련로) 등록. 상태는 여기서 들고, 바뀌면 건물 Actor의 표시용 복제 값을 고친다
	int32 AddMachine(const FJ3MachineState& InitialState, AJ3MachineBuilding* Owner);
	void RemoveMachine(int32 MachineId);
	FJ3MachineState* FindMachineMutable(int32 MachineId);

	// 플레이어가 넣고 꺼낸 뒤처럼 상태를 바꿨으면 부른다 (건물 표시 갱신)
	void NotifyMachineChanged(int32 MachineId);

	// 서버: 기계 출력 → 컨베이어 시작, 컨베이어 끝 → 기계 입력·창고 (컨베이어 건물이 잇고 끊는다)
	void SetMachineOutput(int32 MachineId, int32 SegmentId);
	void SetSegmentSinkMachine(int32 SegmentId, int32 MachineId);
	void SetSegmentSinkStorage(int32 SegmentId, AJ3StorageBuilding* Storage);
	void ClearSegmentLinks(int32 SegmentId);

	// 이 기계(건물 id)에서 이 재료로 만드는 레시피 (recipes.csv의 station)
	static const FJ3RecipeDef* FindMachineRecipe(FName BuildingId, FName InputItem);

private:
	void StepSimulation();
	void StepMachines();
	void TransferBeltsAndMachines();
	void RebuildIndex();

	// 서버: 변경된 구간을 해당 청크의 복제 액터에 알리고, 구간 배치가 바뀌면 청크를 다시 나눈다
	void NotifySegmentsChanged(const TSet<int32>& SegmentIds);
	void RebuildReplicationChunks();
	FIntPoint GetChunkOf(const FJ3BeltSegment& Segment) const;

	TArray<FJ3BeltSegment> Segments;
	TMap<int32, int32> IndexById;

	// 서버 전용: 기계 상태 (Id 오름차순, 처리 순서 고정)
	TArray<FJ3MachineState> Machines;
	TMap<int32, TWeakObjectPtr<AJ3MachineBuilding>> MachineOwners;

	// 서버 전용: 기계 → 출력 구간, 구간 끝 → 받는 쪽 (기계 또는 창고)
	struct FSegmentSink
	{
		int32 MachineId = INDEX_NONE;
		TWeakObjectPtr<AJ3StorageBuilding> Storage;
	};
	TMap<int32, int32> MachineOutputs;
	TMap<int32, FSegmentSink> SegmentSinks;
	int32 NextMachineId = 1;

	// 컨베이어 건물 구간 번호 (벤치마크 구간과 겹치지 않게 큰 수부터)
	int32 NextSegmentId = 1000000;

	float FixedStepSec = 0.05f;
	int32 MaxStepsPerFrame = 5;
	float Accumulator = 0.f;
	int32 TopologyVersion = 0;

	// 서버 전용: 공간 청크별 복제 액터
	UPROPERTY()
	TMap<FIntPoint, TObjectPtr<AJ3FactoryReplicator>> ChunkReplicators;

	TMap<int32, FIntPoint> SegmentChunks;
	int32 ReplicatedTopologyVersion = INDEX_NONE;
	float ChunkSizeCm = 5000.f;
	double StepMsSum = 0.0;
	double StepMsMax = 0.0;
	int32 StepCount = 0;
};
