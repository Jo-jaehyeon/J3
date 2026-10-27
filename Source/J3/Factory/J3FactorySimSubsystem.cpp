// Copyright (c) ACCENT. All Rights Reserved.

#include "Factory/J3FactorySimSubsystem.h"

#include "Building/J3MachineBuildings.h"
#include "Building/J3StorageBuilding.h"
#include "Factory/J3BeltLogic.h"
#include "Data/J3DataSubsystem.h"
#include "Factory/J3MachineLogic.h"

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
	if (GetWorld()->GetNetMode() != NM_Client)
	{
		StepMachines();
	}
}


void UJ3FactorySimSubsystem::StepMachines()
{
	// 컨베이어와 주고받기를 먼저 하고 기계를 돌린다
	TransferBeltsAndMachines();


	// 망·순서가 바뀌었으면 다시 정한다
	if (bPowerGridsDirty)
	{
		RebuildPowerGrids();
	}
	if (bMachineOrderDirty)
	{
		RebuildMachineOrder();
	}


	// 기계 상태는 서버만 계산한다 (클라이언트는 복제된 칸 내용과 주기 시작 시각만 본다).
	// 발전소가 먼저 축전하고, 전기를 쓰는 기계는 상류부터 축전을 꺼내 쓴다 (모자라면 뒤쪽이 멈춘다)
	const UJ3DataSubsystem* Data = UJ3DataSubsystem::Get();
	for (const int32 Index : MachineOrder)
	{
		FJ3MachineState& State = Machines[Index];
		const FJ3MachineDef* Def = Data->GetMachine(State.BuildingId);
		if (Def == nullptr)
		{
			continue;
		}
		const FName BuildingId = State.BuildingId;
		const int32 MachineId = State.Id;
		bool bChanged = J3MachineLogic::Step(State, *Def, FixedStepSec,
			[BuildingId](FName Item) { return FindMachineRecipe(BuildingId, Item); },
			[Data](FName Item) { const FJ3FuelDef* Fuel = Data->GetFuel(Item); return Fuel != nullptr ? Fuel->BurnSec : 0.f; },
			[this, MachineId](float Amount) { return TryConsumePower(MachineId, Amount); });
		if (bChanged)
		{
			NotifyMachineChanged(State.Id);
		}
	}


	// 발전소 축전 비율이 1% 넘게 바뀌면 표시를 고친다 (쓰는 쪽이 꺼내 간 것까지)
	for (FJ3MachineState& State : Machines)
	{
		const FJ3MachineDef* Def = Data->GetMachine(State.BuildingId);
		const int32 Percent = Def != nullptr && Def->IsGenerator() && Def->EnergyCap > 0.f ? FMath::FloorToInt(State.EnergyStored / Def->EnergyCap * 100.f) : INDEX_NONE;
		if (Percent != State.SentEnergyPercent)
		{
			State.SentEnergyPercent = Percent;
			NotifyMachineChanged(State.Id);
		}
	}
}


int32 UJ3FactorySimSubsystem::AddPowerNode(int32 MachineId)
{
	const int32 NodeId = NextPowerNodeId++;
	PowerNodes.Add(NodeId, MachineId);
	bPowerGridsDirty = true;
	return NodeId;
}


void UJ3FactorySimSubsystem::RemovePowerNode(int32 NodeId)
{
	// 이 노드에 이어진 전기줄도 없앤다
	PowerNodes.Remove(NodeId);
	for (auto It = PowerEdges.CreateIterator(); It; ++It)
	{
		if (It.Value().Key == NodeId || It.Value().Value == NodeId)
		{
			It.RemoveCurrent();
		}
	}
	bPowerGridsDirty = true;
}


int32 UJ3FactorySimSubsystem::AddPowerEdge(int32 NodeA, int32 NodeB)
{
	const int32 EdgeId = NextPowerEdgeId++;
	PowerEdges.Add(EdgeId, TPair<int32, int32>(NodeA, NodeB));
	bPowerGridsDirty = true;
	return EdgeId;
}


void UJ3FactorySimSubsystem::RemovePowerEdge(int32 EdgeId)
{
	PowerEdges.Remove(EdgeId);
	bPowerGridsDirty = true;
}


void UJ3FactorySimSubsystem::RebuildPowerGrids()
{
	// 전기줄로 이어진 노드끼리 같은 망 (합집합 찾기). 망 번호는 그 망의 가장 작은 노드 번호
	TMap<int32, int32> Parent;
	for (const TPair<int32, int32>& Node : PowerNodes)
	{
		Parent.Add(Node.Key, Node.Key);
	}
	auto Find = [&Parent](int32 Node)
	{
		int32 Root = Node;
		while (Parent[Root] != Root)
		{
			Root = Parent[Root];
		}
		Parent[Node] = Root;
		return Root;
	};
	for (const TPair<int32, TPair<int32, int32>>& Edge : PowerEdges)
	{
		if (Parent.Contains(Edge.Value.Key) && Parent.Contains(Edge.Value.Value))
		{
			const int32 A = Find(Edge.Value.Key);
			const int32 B = Find(Edge.Value.Value);
			Parent[FMath::Max(A, B)] = FMath::Min(A, B);
		}
	}


	// 기계 → 망, 망 → 발전소 (축전은 망 안의 발전소들을 하나의 풀로 합쳐 쓴다, 제안)
	const UJ3DataSubsystem* Data = UJ3DataSubsystem::Get();
	MachineGrids.Reset();
	GridGenerators.Reset();
	for (const TPair<int32, int32>& Node : PowerNodes)
	{
		if (Node.Value == INDEX_NONE)
		{
			continue;
		}
		const int32 Grid = Find(Node.Key);
		MachineGrids.Add(Node.Value, Grid);
		const FJ3MachineState* State = FindMachineMutable(Node.Value);
		const FJ3MachineDef* Def = State != nullptr ? Data->GetMachine(State->BuildingId) : nullptr;
		if (Def != nullptr && Def->IsGenerator())
		{
			GridGenerators.FindOrAdd(Grid).Add(Node.Value);
		}
	}
	bPowerGridsDirty = false;
}


void UJ3FactorySimSubsystem::RebuildMachineOrder()
{
	// 컨베이어로 이어진 기계 사이의 상류 → 하류 (출력 구간을 따라가다 받는 기계를 만나면 그 기계가 하류)
	TMultiMap<int32, int32> Downstream;
	for (const TPair<int32, int32>& Output : MachineOutputs)
	{
		int32 SegmentId = Output.Value;
		for (int32 Guard = 0; Guard < Segments.Num() && SegmentId != INDEX_NONE; ++Guard)
		{
			const FSegmentSink* Sink = SegmentSinks.Find(SegmentId);
			if (Sink != nullptr)
			{
				if (Sink->MachineId != INDEX_NONE)
				{
					Downstream.Add(Output.Key, Sink->MachineId);
				}
				break;
			}
			const FJ3BeltSegment* Segment = FindSegment(SegmentId);
			SegmentId = Segment != nullptr ? Segment->NextId : INDEX_NONE;
		}
	}


	// 깊이 = 상류에서 몇 번째인지 (고리가 있어도 끝나도록 기계 수만큼만 되풀이). 발전소는 맨 앞
	const UJ3DataSubsystem* Data = UJ3DataSubsystem::Get();
	TMap<int32, int32> Depth;
	for (const FJ3MachineState& State : Machines)
	{
		const FJ3MachineDef* Def = Data->GetMachine(State.BuildingId);
		Depth.Add(State.Id, Def != nullptr && Def->IsGenerator() ? -1 : 0);
	}
	for (int32 Pass = 0; Pass < Machines.Num(); ++Pass)
	{
		bool bMoved = false;
		for (const TPair<int32, int32>& Link : Downstream)
		{
			const int32* From = Depth.Find(Link.Key);
			int32* To = Depth.Find(Link.Value);
			if (From != nullptr && To != nullptr && *To < *From + 1)
			{
				*To = *From + 1;
				bMoved = true;
			}
		}
		if (!bMoved)
		{
			break;
		}
	}


	// 깊이, 같으면 번호 순서
	MachineOrder.Reset(Machines.Num());
	for (int32 Index = 0; Index < Machines.Num(); ++Index)
	{
		MachineOrder.Add(Index);
	}
	MachineOrder.Sort([this, &Depth](int32 A, int32 B)
	{
		const int32 DepthA = Depth.FindRef(Machines[A].Id);
		const int32 DepthB = Depth.FindRef(Machines[B].Id);
		return DepthA != DepthB ? DepthA < DepthB : Machines[A].Id < Machines[B].Id;
	});
	bMachineOrderDirty = false;
}


bool UJ3FactorySimSubsystem::TryConsumePower(int32 MachineId, float Amount)
{
	// 망에 이어진 발전소들의 축전을 합쳐 모자라지 않을 때만 꺼낸다 (번호 순서로 덜어낸다)
	const int32* Grid = MachineGrids.Find(MachineId);
	const TArray<int32>* Generators = Grid != nullptr ? GridGenerators.Find(*Grid) : nullptr;
	if (Generators == nullptr)
	{
		return false;
	}
	float Available = 0.f;
	for (const int32 GeneratorId : *Generators)
	{
		const FJ3MachineState* Generator = FindMachineMutable(GeneratorId);
		Available += Generator != nullptr ? Generator->EnergyStored : 0.f;
	}
	if (Available < Amount)
	{
		return false;
	}
	float Remaining = Amount;
	for (const int32 GeneratorId : *Generators)
	{
		FJ3MachineState* Generator = FindMachineMutable(GeneratorId);
		const float Taken = Generator != nullptr ? FMath::Min(Generator->EnergyStored, Remaining) : 0.f;
		if (Generator != nullptr)
		{
			Generator->EnergyStored -= Taken;
		}
		Remaining -= Taken;
	}
	return true;
}


int32 UJ3FactorySimSubsystem::AddMachine(const FJ3MachineState& InitialState, AJ3MachineBuilding* Owner)
{
	// 새 Id는 늘 가장 크므로 끝에 붙이면 순서가 유지된다
	FJ3MachineState& State = Machines.Add_GetRef(InitialState);
	State.Id = NextMachineId++;
	MachineOwners.Add(State.Id, Owner);
	bMachineOrderDirty = true;
	bPowerGridsDirty = true;
	NotifyMachineChanged(State.Id);
	return State.Id;
}


void UJ3FactorySimSubsystem::RemoveMachine(int32 MachineId)
{
	// 이 기계로 이어진 연결도 끊는다
	Machines.RemoveAll([MachineId](const FJ3MachineState& State) { return State.Id == MachineId; });
	MachineOwners.Remove(MachineId);
	MachineOutputs.Remove(MachineId);
	bMachineOrderDirty = true;
	bPowerGridsDirty = true;
	for (auto It = SegmentSinks.CreateIterator(); It; ++It)
	{
		if (It.Value().MachineId == MachineId)
		{
			It.RemoveCurrent();
		}
	}
}


void UJ3FactorySimSubsystem::SetMachineOutput(int32 MachineId, int32 SegmentId)
{
	bMachineOrderDirty = true;
	MachineOutputs.Add(MachineId, SegmentId);
}


void UJ3FactorySimSubsystem::SetSegmentSinkMachine(int32 SegmentId, int32 MachineId)
{
	bMachineOrderDirty = true;
	FSegmentSink Sink;
	Sink.MachineId = MachineId;
	SegmentSinks.Add(SegmentId, Sink);
}


void UJ3FactorySimSubsystem::SetSegmentSinkStorage(int32 SegmentId, AJ3StorageBuilding* Storage)
{
	FSegmentSink Sink;
	Sink.Storage = Storage;
	SegmentSinks.Add(SegmentId, Sink);
}


void UJ3FactorySimSubsystem::ClearSegmentLinks(int32 SegmentId)
{
	bMachineOrderDirty = true;
	SegmentSinks.Remove(SegmentId);
	for (auto It = MachineOutputs.CreateIterator(); It; ++It)
	{
		if (It.Value() == SegmentId)
		{
			It.RemoveCurrent();
		}
	}
}


void UJ3FactorySimSubsystem::TransferBeltsAndMachines()
{
	const UJ3DataSubsystem* Data = UJ3DataSubsystem::Get();
	TSet<int32> Changed;


	// 컨베이어 끝에 닿은 맨 앞 아이템을 받는 쪽에 넣는다. 받지 못하면(칸이 차거나 맞지 않는 재료) 끝에서 기다린다
	for (const TPair<int32, FSegmentSink>& Pair : SegmentSinks)
	{
		FJ3BeltSegment* Segment = FindSegmentMutable(Pair.Key);
		if (Segment == nullptr || Segment->Items.Num() == 0 || Segment->Items[0].Offset < Segment->LengthCm - KINDA_SMALL_NUMBER)
		{
			continue;
		}
		const FName ItemId = Data->GetItemIdByType(Segment->Items[0].Type);
		bool bAccepted = false;
		FJ3MachineState* State = Pair.Value.MachineId != INDEX_NONE ? FindMachineMutable(Pair.Value.MachineId) : nullptr;
		const FJ3MachineDef* Def = State != nullptr ? Data->GetMachine(State->BuildingId) : nullptr;
		if (Def != nullptr)
		{
			const FName BuildingId = State->BuildingId;
			bAccepted = J3MachineLogic::Insert(*State, *Def, ItemId, 1,
				[BuildingId](FName Item) { return FindMachineRecipe(BuildingId, Item); },
				[Data](FName Item) { const FJ3FuelDef* Fuel = Data->GetFuel(Item); return Fuel != nullptr ? Fuel->BurnSec : 0.f; }) == 1;
			if (bAccepted)
			{
				NotifyMachineChanged(State->Id);
			}
		}
		else if (Pair.Value.Storage.IsValid())
		{
			bAccepted = Pair.Value.Storage->AddItem(ItemId, 1) == 0;
		}
		if (bAccepted)
		{
			Segment->Items.RemoveAt(0);
			Changed.Add(Segment->Id);
		}
	}


	// 기계 출력 칸에서 한 틱에 하나씩 컨베이어 시작에 올린다 (자리가 없으면 출력 칸에 계속 쌓인다)
	for (const TPair<int32, int32>& Pair : MachineOutputs)
	{
		FJ3MachineState* State = FindMachineMutable(Pair.Key);
		FJ3BeltSegment* Segment = FindSegmentMutable(Pair.Value);
		if (State == nullptr || Segment == nullptr || State->Output.IsEmpty())
		{
			continue;
		}
		if (J3BeltLogic::TryInsert(*Segment, Data->GetItemTypeId(State->Output.ItemId)))
		{
			J3MachineLogic::Take(*State, EJ3MachineSlot::Output, 1);
			NotifyMachineChanged(State->Id);
			Changed.Add(Segment->Id);
		}
	}
	if (Changed.Num() > 0)
	{
		NotifySegmentsChanged(Changed);
	}
}


FJ3MachineState* UJ3FactorySimSubsystem::FindMachineMutable(int32 MachineId)
{
	return Machines.FindByPredicate([MachineId](const FJ3MachineState& State) { return State.Id == MachineId; });
}


void UJ3FactorySimSubsystem::NotifyMachineChanged(int32 MachineId)
{
	const FJ3MachineState* State = FindMachineMutable(MachineId);
	const TWeakObjectPtr<AJ3MachineBuilding>* Owner = MachineOwners.Find(MachineId);
	if (State != nullptr && Owner != nullptr && Owner->IsValid())
	{
		(*Owner)->ApplyMachineState(*State);
	}
}


const FJ3RecipeDef* UJ3FactorySimSubsystem::FindMachineRecipe(FName BuildingId, FName InputItem)
{
	// 이 건물에서 만들고 이 재료를 쓰는 첫 레시피 (ID 순서와 상관없이 표에 적힌 것 중 하나, 지금은 재료당 하나뿐)
	for (const TPair<FName, FJ3RecipeDef>& Pair : UJ3DataSubsystem::Get()->GetAllRecipes())
	{
		const FJ3RecipeDef& Recipe = Pair.Value;
		const bool bUsesInput = Recipe.Inputs.ContainsByPredicate([InputItem](const FJ3ItemAmount& Amount) { return Amount.ItemId == InputItem; });
		if (Recipe.Stations.Contains(BuildingId) && bUsesInput)
		{
			return &Recipe;
		}
	}
	return nullptr;
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


void UJ3FactorySimSubsystem::SetSegmentNext(int32 SegmentId, int32 NextId)
{
	FJ3BeltSegment* Segment = FindSegmentMutable(SegmentId);
	if (Segment != nullptr && Segment->NextId != NextId)
	{
		Segment->NextId = NextId;
		bMachineOrderDirty = true;
		NotifySegmentsChanged({ SegmentId });
	}
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
