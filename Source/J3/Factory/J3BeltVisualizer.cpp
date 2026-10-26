// Copyright (c) ACCENT. All Rights Reserved.

#include "Factory/J3BeltVisualizer.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/World.h"
#include "Factory/J3FactorySimSubsystem.h"


AJ3BeltVisualizer::AJ3BeltVisualizer()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = false;


	// 충돌 없는 표시 전용 인스턴스 묶음 두 개 (컨베이어, 아이템)
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	BeltInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("BeltInstances"));
	BeltInstances->SetupAttachment(RootComponent);
	BeltInstances->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ItemInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("ItemInstances"));
	ItemInstances->SetupAttachment(RootComponent);
	ItemInstances->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ItemInstances->SetCastShadow(false);
}


void AJ3BeltVisualizer::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const UJ3FactorySimSubsystem* Simulation = GetWorld()->GetSubsystem<UJ3FactorySimSubsystem>();
	if (Simulation == nullptr)
	{
		return;
	}
	if (BuiltTopologyVersion != Simulation->GetTopologyVersion())
	{
		RebuildBelts(*Simulation);
	}
	UpdateItems(*Simulation);
}


void AJ3BeltVisualizer::RebuildBelts(const UJ3FactorySimSubsystem& Simulation)
{
	// 구간마다 메시 하나를 시작~끝 사이에 늘여 놓는다 (구간이 바뀔 때만)
	TArray<FTransform> Transforms;
	for (const FJ3BeltSegment& Segment : Simulation.GetSegments())
	{
		const FVector Direction = Segment.EndLocation - Segment.StartLocation;
		const FVector Scale(Direction.Size() / BeltMeshLengthCm * BeltScale.X, BeltScale.Y, BeltScale.Z);
		Transforms.Add(FTransform(Direction.Rotation(), (Segment.StartLocation + Segment.EndLocation) * 0.5f, Scale));
	}
	BeltInstances->ClearInstances();
	BeltInstances->AddInstances(Transforms, false, true);
	BuiltTopologyVersion = Simulation.GetTopologyVersion();
}


void AJ3BeltVisualizer::UpdateItems(const UJ3FactorySimSubsystem& Simulation)
{
	// 시뮬레이션은 고정 틱이라, 지난 틱 이후 시간만큼 앞으로 그려 움직임을 매끄럽게 한다
	const float SinceStep = Simulation.GetTimeSinceStep();
	ItemTransforms.Reset();
	for (const FJ3BeltSegment& Segment : Simulation.GetSegments())
	{
		const FVector Direction = (Segment.EndLocation - Segment.StartLocation).GetSafeNormal();
		const FRotator Rotation = Direction.Rotation();
		const FVector Up(0.f, 0.f, ItemHeightCm);
		for (const FJ3BeltItem& Item : Segment.Items)
		{
			const float Offset = FMath::Min(Item.Offset + Segment.SpeedCmPerSec * SinceStep, Segment.LengthCm);
			ItemTransforms.Emplace(Rotation, Segment.StartLocation + Direction * Offset + Up, ItemScale);
		}
	}


	// 개수가 바뀌었으면 맞추고, 위치는 한 번에 갱신한다
	const int32 CurrentCount = ItemInstances->GetInstanceCount();
	if (CurrentCount < ItemTransforms.Num())
	{
		TArray<FTransform> Added;
		Added.Init(FTransform::Identity, ItemTransforms.Num() - CurrentCount);
		ItemInstances->AddInstances(Added, false, true);
	}
	else if (CurrentCount > ItemTransforms.Num())
	{
		TArray<int32> Removed;
		for (int32 Index = CurrentCount - 1; Index >= ItemTransforms.Num(); --Index)
		{
			Removed.Add(Index);
		}
		ItemInstances->RemoveInstances(Removed, true);
	}
	ItemInstances->BatchUpdateInstancesTransforms(0, ItemTransforms, true, true, true);
}
