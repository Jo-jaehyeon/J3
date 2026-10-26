// Copyright (c) ACCENT. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Factory/J3BeltTypes.h"
#include "J3FactorySimSubsystem.generated.h"

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
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;


	// 구간 추가·삭제 (Id 오름차순을 유지해 서버·클라이언트의 처리 순서를 같게 한다)
	void AddSegment(const FJ3BeltSegment& Segment);
	void RemoveSegment(int32 SegmentId);
	void ClearAll();

	bool InsertItem(int32 SegmentId, FJ3ItemTypeId Type);

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

	// 측정: 최근 고정 틱 한 번의 계산 시간 (ms)
	double GetLastStepMs() const
	{
		return LastStepMs;
	}

	// 아이템이 들어오거나 나간 구간 (복제용)
	FJ3OnSegmentsChanged OnSegmentsChanged;

private:
	void StepSimulation();
	void RebuildIndex();

	TArray<FJ3BeltSegment> Segments;
	TMap<int32, int32> IndexById;

	float FixedStepSec = 0.05f;
	int32 MaxStepsPerFrame = 5;
	float Accumulator = 0.f;
	double LastStepMs = 0.0;
};
