// Copyright (c) ACCENT. All Rights Reserved.

#include "Building/J3ConveyorBuilding.h"

#include "Building/J3BuildingSubsystem.h"
#include "Building/J3MachineBuildings.h"
#include "Building/J3StorageBuilding.h"
#include "Components/BoxComponent.h"
#include "Data/J3DataSubsystem.h"
#include "Engine/World.h"
#include "Factory/J3FactorySimSubsystem.h"
#include "Net/UnrealNetwork.h"


namespace J3ConveyorBuildingPrivate
{
	// 충돌 상자 한 개가 덮는 길이와 단면 반 크기 (벨트 표시 크기와 비슷하게)
	constexpr float BoxLengthCm = 100.f;
	constexpr float BoxHalfWidthCm = 30.f;
	constexpr float BoxHalfHeightCm = 10.f;

	// 두 끝점이 이만큼 가까우면 같은 점 (양자화 오차 포함)
	constexpr float LinkToleranceCm = 2.f;
}


FJ3ConveyorPath FJ3ConveyorPathNet::ToPath() const
{
	FJ3ConveyorPath Path;
	Path.Start = Start;
	Path.End = End;
	Path.StartDirection = StartDirection;
	Path.EndDirection = EndDirection;
	return Path;
}


FJ3ConveyorPathNet FJ3ConveyorPathNet::FromPath(const FJ3ConveyorPath& Path)
{
	FJ3ConveyorPathNet Net;
	Net.Start = Path.Start;
	Net.End = Path.End;
	Net.StartDirection = Path.StartDirection;
	Net.EndDirection = Path.EndDirection;
	return Net;
}


AJ3ConveyorBuilding::AJ3ConveyorBuilding()
{
	// 표시는 공장 표시가 맡아 메시는 쓰지 않는다
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetVisibility(false);
}


void AJ3ConveyorBuilding::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AJ3ConveyorBuilding, PathNet);
}


void AJ3ConveyorBuilding::SetPath(const FJ3ConveyorPath& InPath)
{
	PathNet = FJ3ConveyorPathNet::FromPath(InPath);
}


void AJ3ConveyorBuilding::BeginPlay()
{
	Super::BeginPlay();
	RebuildCollision();


	// 서버: 곡선 길이로 시뮬레이션 구간을 만든다 (클라이언트는 복제 액터로 구간을 받는다)
	UJ3FactorySimSubsystem* Sim = GetWorld()->GetSubsystem<UJ3FactorySimSubsystem>();
	if (!HasAuthority() || Sim == nullptr)
	{
		return;
	}
	const UJ3DataSubsystem* Data = UJ3DataSubsystem::Get();
	const FJ3ConveyorPath Path = GetPath();
	FJ3BeltSegment Segment;
	Segment.Id = Sim->AllocateSegmentId();
	Segment.LengthCm = Path.ComputeLength();
	Segment.SpeedCmPerSec = Data->GetConfigFloat("conveyor_speed_cm_per_sec", 100.f);
	Segment.SpacingCm = Data->GetConfigFloat("conveyor_spacing_cm", 25.f);
	Segment.StartLocation = Path.Start;
	Segment.EndLocation = Path.End;
	Segment.StartDirection = Path.StartDirection;
	Segment.EndDirection = Path.EndDirection;
	Sim->AddSegment(Segment);
	SegmentId = Segment.Id;
	LinkNeighbors();
}


void AJ3ConveyorBuilding::LinkNeighbors()
{
	using namespace J3ConveyorBuildingPrivate;

	// 내 끝 = 다른 컨베이어 시작이면 그쪽으로 넘기고, 다른 컨베이어 끝 = 내 시작이면 그쪽이 나에게 넘긴다
	UJ3FactorySimSubsystem* Sim = GetWorld()->GetSubsystem<UJ3FactorySimSubsystem>();
	const UJ3BuildingSubsystem* Buildings = GetWorld()->GetSubsystem<UJ3BuildingSubsystem>();
	const FJ3ConveyorPath Path = GetPath();
	const float SearchCm = UJ3DataSubsystem::Get()->GetConfigFloat("conveyor_max_length_cm", 3000.f) + LinkToleranceCm;
	for (AJ3BuildingBase* Other : Buildings->FindBuildingsInBox(FBox(Path.Start - FVector(SearchCm), Path.Start + FVector(SearchCm))))
	{
		// 기계·창고: 시작이 출력 포트면 기계가 여기로 내보내고, 끝이 입력 포트면 여기서 받는다
		const AJ3ConveyorBuilding* Conveyor = Cast<AJ3ConveyorBuilding>(Other);
		if (Conveyor == nullptr && Other->CanConnectConveyor())
		{
			TArray<FJ3PortInfo> Ports;
			Other->GetPorts(Ports);
			for (const FJ3PortInfo& Port : Ports)
			{
				const FVector PortLocation = Port.Transform.GetLocation();
				AJ3MachineBuilding* Machine = Cast<AJ3MachineBuilding>(Other);
				AJ3StorageBuilding* Storage = Cast<AJ3StorageBuilding>(Other);
				if (Port.Kind != EJ3PortKind::Item)
				{
					continue;
				}
				if (Port.Direction == EJ3PortDirection::Out && Machine != nullptr && FVector::Dist(PortLocation, Path.Start) <= LinkToleranceCm)
				{
					Sim->SetMachineOutput(Machine->GetMachineId(), SegmentId);
				}
				if (Port.Direction == EJ3PortDirection::In && FVector::Dist(PortLocation, Path.End) <= LinkToleranceCm)
				{
					if (Machine != nullptr)
					{
						Sim->SetSegmentSinkMachine(SegmentId, Machine->GetMachineId());
					}
					else if (Storage != nullptr)
					{
						Sim->SetSegmentSinkStorage(SegmentId, Storage);
					}
				}
			}
			continue;
		}
		if (Conveyor == nullptr || Conveyor == this || Conveyor->GetSegmentId() == INDEX_NONE)
		{
			continue;
		}
		const FJ3ConveyorPath OtherPath = Conveyor->GetPath();
		if (FVector::Dist(OtherPath.Start, Path.End) <= LinkToleranceCm)
		{
			Sim->SetSegmentNext(SegmentId, Conveyor->GetSegmentId());
		}
		if (FVector::Dist(OtherPath.End, Path.Start) <= LinkToleranceCm)
		{
			Sim->SetSegmentNext(Conveyor->GetSegmentId(), SegmentId);
		}
	}
}


void AJ3ConveyorBuilding::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 서버: 나에게 넘기던 구간의 다음을 끊고 내 구간을 없앤다
	UJ3FactorySimSubsystem* Sim = GetWorld()->GetSubsystem<UJ3FactorySimSubsystem>();
	if (Sim != nullptr && SegmentId != INDEX_NONE)
	{
		for (const FJ3BeltSegment& Segment : TArray<FJ3BeltSegment>(Sim->GetSegments()))
		{
			if (Segment.NextId == SegmentId)
			{
				Sim->SetSegmentNext(Segment.Id, INDEX_NONE);
			}
		}
		Sim->ClearSegmentLinks(SegmentId);
		Sim->RemoveSegment(SegmentId);
	}
	Super::EndPlay(EndPlayReason);
}


void AJ3ConveyorBuilding::RelinkConveyorsNear(const AJ3BuildingBase* Building)
{
	// 컨베이어는 시작점에 등록되어 있어 최대 길이만큼 넓게 찾는다
	const UJ3BuildingSubsystem* Buildings = Building->GetWorld()->GetSubsystem<UJ3BuildingSubsystem>();
	const float SearchCm = UJ3DataSubsystem::Get()->GetConfigFloat("conveyor_max_length_cm", 3000.f) + Building->GetFootprintHalfExtent().GetMax();
	const FVector Center = Building->GetActorLocation();
	for (AJ3BuildingBase* Other : Buildings->FindBuildingsInBox(FBox(Center - FVector(SearchCm), Center + FVector(SearchCm))))
	{
		AJ3ConveyorBuilding* Conveyor = Cast<AJ3ConveyorBuilding>(Other);
		if (Conveyor != nullptr && Conveyor->GetSegmentId() != INDEX_NONE)
		{
			Conveyor->LinkNeighbors();
		}
	}
}


void AJ3ConveyorBuilding::OnRep_PathNet()
{
	RebuildCollision();
}


void AJ3ConveyorBuilding::RebuildCollision()
{
	using namespace J3ConveyorBuildingPrivate;

	// 곡선을 일정 길이로 나눠 조각마다 상자 하나 (조준 Visibility는 막고, 이동은 막지 않는다)
	for (UBoxComponent* Box : CollisionBoxes)
	{
		Box->DestroyComponent();
	}
	CollisionBoxes.Reset();
	const FJ3ConveyorPath Path = GetPath();
	if (Path.Start.Equals(Path.End))
	{
		return;
	}
	TArray<FVector> Points;
	TArray<float> Distances;
	Path.BuildArcTable(FMath::CeilToInt(Path.ComputeLength() / BoxLengthCm) + 1, Points, Distances);
	for (int32 Index = 1; Index < Points.Num(); ++Index)
	{
		const FVector Piece = Points[Index] - Points[Index - 1];
		UBoxComponent* Box = NewObject<UBoxComponent>(this);
		Box->SetupAttachment(RootComponent);
		Box->SetCollisionProfileName(UCollisionProfile::BlockAllDynamic_ProfileName);
		Box->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		Box->SetBoxExtent(FVector(Piece.Size() * 0.5f, BoxHalfWidthCm, BoxHalfHeightCm));
		Box->SetWorldLocationAndRotation((Points[Index] + Points[Index - 1]) * 0.5f, Piece.Rotation());
		Box->RegisterComponent();
		CollisionBoxes.Add(Box);
	}
}


void AJ3ConveyorBuilding::GetStoredItems(TArray<FJ3ItemStack>& OutItems) const
{
	// 서버: 벨트 위 아이템을 종류별로 모은다
	const UJ3FactorySimSubsystem* Sim = GetWorld()->GetSubsystem<UJ3FactorySimSubsystem>();
	const FJ3BeltSegment* Segment = Sim != nullptr ? Sim->FindSegment(SegmentId) : nullptr;
	if (Segment == nullptr)
	{
		return;
	}
	TMap<FName, int32> Counts;
	for (const FJ3BeltItem& Item : Segment->Items)
	{
		Counts.FindOrAdd(UJ3DataSubsystem::Get()->GetItemIdByType(Item.Type)) += 1;
	}
	for (const TPair<FName, int32>& Pair : Counts)
	{
		FJ3ItemStack Stack;
		Stack.ItemId = Pair.Key;
		Stack.Count = Pair.Value;
		OutItems.Add(Stack);
	}
}
