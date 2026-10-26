// Copyright (c) ACCENT. All Rights Reserved.

#include "Factory/J3FactorySimSubsystem.h"

#include "Algo/BinarySearch.h"
#include "Data/J3DataSubsystem.h"
#include "Engine/World.h"
#include "Factory/J3BeltVisualizer.h"
#include "Factory/J3FactoryBench.h"
#include "Factory/J3FactoryReplicator.h"
#include "J3ProjectSettings.h"
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
	ChunkSizeCm = FMath::Max(100.f, Data->GetConfigFloat("factory_chunk_size_cm", ChunkSizeCm));
}


void UJ3FactorySimSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	// 화면이 있는 쪽(리슨 서버 호스트·클라이언트)만 컨베이어 아이템을 그린다
	if (InWorld.GetNetMode() != NM_DedicatedServer)
	{
		const TSubclassOf<AJ3BeltVisualizer> VisualizerClass = GetDefault<UJ3ProjectSettings>()->BeltVisualizerClass.LoadSynchronous();
		FActorSpawnParameters Params;
		Params.ObjectFlags |= RF_Transient;
		InWorld.SpawnActor<AJ3BeltVisualizer>(VisualizerClass != nullptr ? VisualizerClass : TSubclassOf<AJ3BeltVisualizer>(AJ3BeltVisualizer::StaticClass()), Params);
	}


	// 클라이언트는 청크별 복제 액터를 서버에서 받는다
	if (InWorld.GetNetMode() == NM_Client)
	{
		J3FactoryBench::StartClientStatsFromConsoleVariable(*this);
		return;
	}


	// 개발용: 시작하자마자 벤치마크 컨베이어를 깐다
	J3FactoryBench::SpawnFromConsoleVariable(*this);
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


	// 서버: 구간 배치가 바뀌었으면 청크별 복제 액터를 다시 맞춘다
	if (GetWorld()->GetNetMode() != NM_Client && ReplicatedTopologyVersion != TopologyVersion)
	{
		RebuildReplicationChunks();
	}
}


FIntPoint UJ3FactorySimSubsystem::GetChunkOf(const FJ3BeltSegment& Segment) const
{
	// 구간 시작점이 있는 격자 칸
	return FIntPoint(FMath::FloorToInt(Segment.StartLocation.X / ChunkSizeCm), FMath::FloorToInt(Segment.StartLocation.Y / ChunkSizeCm));
}


void UJ3FactorySimSubsystem::RebuildReplicationChunks()
{
	// 구간을 청크로 나눈다
	TMap<FIntPoint, TArray<int32>> SegmentsByChunk;
	SegmentChunks.Reset();
	for (const FJ3BeltSegment& Segment : Segments)
	{
		const FIntPoint Chunk = GetChunkOf(Segment);
		SegmentChunks.Add(Segment.Id, Chunk);
		SegmentsByChunk.FindOrAdd(Chunk).Add(Segment.Id);
	}


	// 비게 된 청크의 복제 액터는 없앤다 (클라이언트는 액터가 사라질 때 구간을 지운다)
	for (auto It = ChunkReplicators.CreateIterator(); It; ++It)
	{
		if (!SegmentsByChunk.Contains(It.Key()))
		{
			if (It.Value() != nullptr)
			{
				It.Value()->Destroy();
			}
			It.RemoveCurrent();
		}
	}


	// 청크마다 복제 액터가 있게 하고 구간 목록을 맞춘다
	for (const TPair<FIntPoint, TArray<int32>>& Pair : SegmentsByChunk)
	{
		TObjectPtr<AJ3FactoryReplicator>& Replicator = ChunkReplicators.FindOrAdd(Pair.Key);
		if (Replicator == nullptr)
		{
			FActorSpawnParameters Params;
			Params.ObjectFlags |= RF_Transient;
			Replicator = GetWorld()->SpawnActor<AJ3FactoryReplicator>(AJ3FactoryReplicator::StaticClass(), Params);
		}
		Replicator->SetSegments(Pair.Value);
	}
	ReplicatedTopologyVersion = TopologyVersion;
}


void UJ3FactorySimSubsystem::NotifySegmentsChanged(const TSet<int32>& SegmentIds)
{
	// 서버: 해당 청크의 복제 액터에 알린다
	if (GetWorld()->GetNetMode() != NM_Client)
	{
		for (const int32 SegmentId : SegmentIds)
		{
			const FIntPoint* Chunk = SegmentChunks.Find(SegmentId);
			const TObjectPtr<AJ3FactoryReplicator>* Replicator = Chunk != nullptr ? ChunkReplicators.Find(*Chunk) : nullptr;
			if (Replicator != nullptr && *Replicator != nullptr)
			{
				(*Replicator)->MarkSegmentDirty(SegmentId);
			}
		}
	}
	OnSegmentsChanged.Broadcast(SegmentIds);
}


void UJ3FactorySimSubsystem::StepSimulation()
{
	// 계산 시간 측정 (단계 2 벤치마크)
	const double StartSeconds = FPlatformTime::Seconds();
	TSet<int32> Changed;
	J3BeltLogic::Step(Segments, IndexById, FixedStepSec, Changed);
	const double StepMs = (FPlatformTime::Seconds() - StartSeconds) * 1000.0;
	StepMsSum += StepMs;
	StepMsMax = FMath::Max(StepMsMax, StepMs);
	++StepCount;


	if (Changed.Num() > 0)
	{
		NotifySegmentsChanged(Changed);
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
	++TopologyVersion;
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
	++TopologyVersion;
}


void UJ3FactorySimSubsystem::ClearAll()
{
	Segments.Reset();
	IndexById.Reset();
	++TopologyVersion;
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
	NotifySegmentsChanged({ SegmentId });
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


void UJ3FactorySimSubsystem::ConsumeStepStats(double& OutAverageMs, double& OutMaxMs)
{
	OutAverageMs = StepCount > 0 ? StepMsSum / StepCount : 0.0;
	OutMaxMs = StepMsMax;
	StepMsSum = 0.0;
	StepMsMax = 0.0;
	StepCount = 0;
}


int32 UJ3FactorySimSubsystem::CountItems() const
{
	return J3BeltLogic::CountItems(Segments);
}
