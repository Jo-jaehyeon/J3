// Copyright (c) ACCENT. All Rights Reserved.

#include "Factory/J3FactorySimSubsystem.h"

#include "Algo/BinarySearch.h"
#include "Data/J3DataSubsystem.h"
#include "Factory/J3BeltLogic.h"
#include "HAL/PlatformTime.h"

DEFINE_LOG_CATEGORY(LogJ3Factory);


bool UJ3FactorySimSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	// 게임 월드(PIE 포함)에만. 에디터 편집 월드에서는 돌리지 않는다.
	const UWorld* World = Cast<UWorld>(Outer);
	return World != nullptr && World->IsGameWorld();
}


void UJ3FactorySimSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// 고정 틱 빈도와 한 프레임 최대 따라잡기 횟수 (느린 프레임에서 계산이 계속 밀리는 것을 막는다)
	const UJ3DataSubsystem* Data = UJ3DataSubsystem::Get();
	FixedStepSec = 1.f / FMath::Max(1.f, Data->GetConfigFloat("sim_tick_hz", 20.f));
	MaxStepsPerFrame = FMath::Max(1, Data->GetConfigInt("sim_max_steps_per_frame", 5));
}


TStatId UJ3FactorySimSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UJ3FactorySimSubsystem, STATGROUP_Tickables);
}


void UJ3FactorySimSubsystem::Tick(float DeltaTime)
{
	// 고정 틱: 쌓인 시간만큼 일정 간격으로 돌린다. 너무 밀리면 버린다.
	Accumulator += DeltaTime;
	int32 Steps = 0;
	while (Accumulator >= FixedStepSec && Steps < MaxStepsPerFrame)
	{
		StepSimulation();
		Accumulator -= FixedStepSec;
		++Steps;
	}
	if (Steps == MaxStepsPerFrame)
	{
		Accumulator = FMath::Min(Accumulator, FixedStepSec);
	}
}


void UJ3FactorySimSubsystem::StepSimulation()
{
	// 계산 시간 측정 (단계 2 벤치마크)
	const double StartSeconds = FPlatformTime::Seconds();
	TSet<int32> Changed;
	J3BeltLogic::Step(Segments, IndexById, FixedStepSec, Changed);
	LastStepMs = (FPlatformTime::Seconds() - StartSeconds) * 1000.0;


	if (Changed.Num() > 0)
	{
		OnSegmentsChanged.Broadcast(Changed);
	}
}


void UJ3FactorySimSubsystem::AddSegment(const FJ3BeltSegment& Segment)
{
	// 같은 Id가 있으면 바꾸고, 없으면 Id 순서 자리에 넣는다
	if (FJ3BeltSegment* Existing = FindSegmentMutable(Segment.Id))
	{
		*Existing = Segment;
		return;
	}
	const int32 InsertAt = Algo::LowerBoundBy(Segments, Segment.Id, &FJ3BeltSegment::Id);
	Segments.Insert(Segment, InsertAt);
	RebuildIndex();
}


void UJ3FactorySimSubsystem::RemoveSegment(int32 SegmentId)
{
	const int32* Index = IndexById.Find(SegmentId);
	if (Index == nullptr)
	{
		return;
	}
	Segments.RemoveAt(*Index);
	RebuildIndex();
}


void UJ3FactorySimSubsystem::ClearAll()
{
	Segments.Reset();
	IndexById.Reset();
}


void UJ3FactorySimSubsystem::RebuildIndex()
{
	IndexById.Reset();
	for (int32 Index = 0; Index < Segments.Num(); ++Index)
	{
		IndexById.Add(Segments[Index].Id, Index);
	}
}


bool UJ3FactorySimSubsystem::InsertItem(int32 SegmentId, FJ3ItemTypeId Type)
{
	FJ3BeltSegment* Segment = FindSegmentMutable(SegmentId);
	if (Segment == nullptr || !J3BeltLogic::TryInsert(*Segment, Type))
	{
		return false;
	}
	OnSegmentsChanged.Broadcast({ SegmentId });
	return true;
}


const FJ3BeltSegment* UJ3FactorySimSubsystem::FindSegment(int32 SegmentId) const
{
	const int32* Index = IndexById.Find(SegmentId);
	return Index != nullptr ? &Segments[*Index] : nullptr;
}


FJ3BeltSegment* UJ3FactorySimSubsystem::FindSegmentMutable(int32 SegmentId)
{
	const int32* Index = IndexById.Find(SegmentId);
	return Index != nullptr ? &Segments[*Index] : nullptr;
}


int32 UJ3FactorySimSubsystem::CountItems() const
{
	return J3BeltLogic::CountItems(Segments);
}
