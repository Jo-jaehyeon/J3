// Copyright (c) ACCENT. All Rights Reserved.

#include "Building/J3PowerBuildings.h"

#include "Building/J3BuildingSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Data/J3DataSubsystem.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Factory/J3FactorySimSubsystem.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"


AJ3PowerPoleBuilding::AJ3PowerPoleBuilding()
{
	// 전기 연결점: 위쪽 (기본 3m 기둥 기준, 블루프린트에서 고친다)
	PowerPort = CreateDefaultSubobject<UJ3PortComponent>(TEXT("PowerPort"));
	PowerPort->SetupAttachment(Root);
	PowerPort->Kind = EJ3PortKind::Power;
	PowerPort->SetRelativeLocation(FVector(0.f, 0.f, 300.f));
}


void AJ3PowerPoleBuilding::BeginPlay()
{
	Super::BeginPlay();
	UJ3FactorySimSubsystem* Sim = GetWorld()->GetSubsystem<UJ3FactorySimSubsystem>();
	if (HasAuthority() && Sim != nullptr)
	{
		PowerNodeId = Sim->AddPowerNode(INDEX_NONE);
	}
}


void AJ3PowerPoleBuilding::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UJ3FactorySimSubsystem* Sim = GetWorld()->GetSubsystem<UJ3FactorySimSubsystem>();
	if (Sim != nullptr && PowerNodeId != INDEX_NONE)
	{
		Sim->RemovePowerNode(PowerNodeId);
	}
	Super::EndPlay(EndPlayReason);
}


AJ3PowerWireBuilding::AJ3PowerWireBuilding()
{
	// 모양: 두 점 사이에 늘인 가는 원기둥. 조준(철거)만 막고 이동은 막지 않는다
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	Mesh->SetStaticMesh(CylinderMesh.Object);
	Mesh->SetCollisionProfileName(UCollisionProfile::BlockAllDynamic_ProfileName);
	Mesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	Mesh->SetCastShadow(false);
	Mesh->SetUsingAbsoluteLocation(true);
	Mesh->SetUsingAbsoluteRotation(true);
	Mesh->SetUsingAbsoluteScale(true);
}


void AJ3PowerWireBuilding::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AJ3PowerWireBuilding, NodeA);
	DOREPLIFETIME(AJ3PowerWireBuilding, NodeB);
	DOREPLIFETIME(AJ3PowerWireBuilding, PointA);
	DOREPLIFETIME(AJ3PowerWireBuilding, PointB);
}


void AJ3PowerWireBuilding::SetEnds(AJ3BuildingBase* InNodeA, AJ3BuildingBase* InNodeB, const FVector& InPointA, const FVector& InPointB)
{
	NodeA = InNodeA;
	NodeB = InNodeB;
	PointA = InPointA;
	PointB = InPointB;
}


void AJ3PowerWireBuilding::BeginPlay()
{
	Super::BeginPlay();
	UpdateShape();


	// 서버: 두 노드를 전력망 간선으로 잇는다
	UJ3FactorySimSubsystem* Sim = GetWorld()->GetSubsystem<UJ3FactorySimSubsystem>();
	if (HasAuthority() && Sim != nullptr && NodeA != nullptr && NodeB != nullptr)
	{
		PowerEdgeId = Sim->AddPowerEdge(NodeA->GetPowerNodeId(), NodeB->GetPowerNodeId());
	}
}


void AJ3PowerWireBuilding::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UJ3FactorySimSubsystem* Sim = GetWorld()->GetSubsystem<UJ3FactorySimSubsystem>();
	if (Sim != nullptr && PowerEdgeId != INDEX_NONE)
	{
		Sim->RemovePowerEdge(PowerEdgeId);
	}
	Super::EndPlay(EndPlayReason);
}


void AJ3PowerWireBuilding::OnRep_Ends()
{
	UpdateShape();
}


void AJ3PowerWireBuilding::UpdateShape()
{
	// 원기둥(높이 100cm, 가운데 원점)을 두 점 사이로 늘여 세운다
	const FVector Span = FVector(PointB) - FVector(PointA);
	if (Span.IsNearlyZero())
	{
		return;
	}
	Mesh->SetWorldLocationAndRotation((FVector(PointA) + FVector(PointB)) * 0.5f, FRotationMatrix::MakeFromZ(Span).ToQuat());
	Mesh->SetWorldScale3D(FVector(Thickness, Thickness, Span.Size() / 100.f));
}


TArray<AJ3PowerWireBuilding*> AJ3PowerWireBuilding::FindWiresOf(const AJ3BuildingBase* Building)
{
	// 전기줄은 시작점에 등록되어 있어 최대 길이만큼 넓게 찾는다
	TArray<AJ3PowerWireBuilding*> Wires;
	const UJ3BuildingSubsystem* Buildings = Building != nullptr ? Building->GetWorld()->GetSubsystem<UJ3BuildingSubsystem>() : nullptr;
	if (Buildings == nullptr)
	{
		return Wires;
	}
	const float SearchCm = UJ3DataSubsystem::Get()->GetConfigFloat("power_pole_wire_max_length_cm", 3000.f) + Building->GetFootprintHalfExtent().GetMax();
	const FVector Center = Building->GetActorLocation();
	for (AJ3BuildingBase* Other : Buildings->FindBuildingsInBox(FBox(Center - FVector(SearchCm), Center + FVector(SearchCm))))
	{
		AJ3PowerWireBuilding* Wire = Cast<AJ3PowerWireBuilding>(Other);
		if (Wire != nullptr && Wire->Connects(Building))
		{
			Wires.Add(Wire);
		}
	}
	return Wires;
}
