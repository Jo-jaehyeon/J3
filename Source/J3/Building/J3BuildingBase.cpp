// Copyright (c) ACCENT. All Rights Reserved.

#include "Building/J3BuildingBase.h"

#include "Building/J3BuildingSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Data/J3DataSubsystem.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"


AJ3BuildingBase::AJ3BuildingBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;


	// 원점 = 바닥 가운데. 메시는 블루프린트에서 원점 위로 올려 놓는다.
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Root);
	Mesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
}


void AJ3BuildingBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AJ3BuildingBase, BuildingId);
}


void AJ3BuildingBase::BeginPlay()
{
	Super::BeginPlay();

	// 서버·클라이언트 모두 공간 인덱스에 등록한다 (클라이언트도 미리보기 판정에 쓴다)
	if (UJ3BuildingSubsystem* Buildings = GetWorld()->GetSubsystem<UJ3BuildingSubsystem>())
	{
		Buildings->RegisterBuilding(this);
	}
}


void AJ3BuildingBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UJ3BuildingSubsystem* Buildings = GetWorld()->GetSubsystem<UJ3BuildingSubsystem>())
	{
		Buildings->UnregisterBuilding(this);
	}
	Super::EndPlay(EndPlayReason);
}


const FJ3BuildingDef* AJ3BuildingBase::GetDef() const
{
	return UJ3DataSubsystem::Get()->GetBuilding(BuildingId);
}


FVector AJ3BuildingBase::GetFootprintHalfExtent() const
{
	// 메시 크기 × 배율. 메시는 원점 위에 놓여 있으므로 높이는 전체 높이의 절반을 반 크기로 쓴다.
	const UStaticMesh* StaticMesh = Mesh->GetStaticMesh();
	if (StaticMesh == nullptr)
	{
		return FVector(50.f);
	}
	return StaticMesh->GetBounds().BoxExtent * Mesh->GetRelativeScale3D();
}
