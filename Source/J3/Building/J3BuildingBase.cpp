// Copyright (c) ACCENT. All Rights Reserved.

#include "Building/J3BuildingBase.h"

#include "Building/J3BuildingSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Data/J3DataSubsystem.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/BlueprintGeneratedClass.h"
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


void AJ3BuildingBase::GetPorts(TArray<FJ3PortInfo>& OutPorts) const
{
	TInlineComponentArray<UJ3PortComponent*> Ports(this);
	for (const UJ3PortComponent* Port : Ports)
	{
		OutPorts.Add({ Port->Kind, Port->Direction, Port->GetComponentTransform() });
	}
}


void AJ3BuildingBase::GetClassPorts(TSubclassOf<AJ3BuildingBase> BuildingClass, TArray<FJ3PortInfo>& OutPorts)
{
	// C++에서 만든 포트는 기본 객체에 있다
	const AJ3BuildingBase* Default = BuildingClass != nullptr ? BuildingClass->GetDefaultObject<AJ3BuildingBase>() : nullptr;
	if (Default == nullptr)
	{
		return;
	}
	TInlineComponentArray<UJ3PortComponent*> NativePorts(Default);
	for (const UJ3PortComponent* Port : NativePorts)
	{
		OutPorts.Add({ Port->Kind, Port->Direction, Port->GetRelativeTransform() });
	}


	// 블루프린트에서 붙인 포트는 구성 스크립트에 있다 (루트에 바로 붙인다고 본다). 부모 블루프린트까지 본다
	for (UClass* Class = BuildingClass; Class != nullptr; Class = Class->GetSuperClass())
	{
		const UBlueprintGeneratedClass* BlueprintClass = Cast<UBlueprintGeneratedClass>(Class);
		const USimpleConstructionScript* Script = BlueprintClass != nullptr ? BlueprintClass->SimpleConstructionScript : nullptr;
		if (Script == nullptr)
		{
			continue;
		}
		for (const USCS_Node* Node : Script->GetAllNodes())
		{
			if (const UJ3PortComponent* Port = Cast<UJ3PortComponent>(Node->ComponentTemplate))
			{
				OutPorts.Add({ Port->Kind, Port->Direction, Port->GetRelativeTransform() });
			}
		}
	}
}


AJ3PlaceholderBuilding::AJ3PlaceholderBuilding()
{
	// 기본 자리 표시 모양(2m 정육면체) 기준 앞뒤 면 가운데
	ItemIn = CreateDefaultSubobject<UJ3PortComponent>(TEXT("ItemIn"));
	ItemIn->SetupAttachment(Root);
	ItemIn->Direction = EJ3PortDirection::In;
	ItemIn->SetRelativeLocationAndRotation(FVector(-100.f, 0.f, 50.f), FRotator(0.f, 180.f, 0.f));
	ItemOut = CreateDefaultSubobject<UJ3PortComponent>(TEXT("ItemOut"));
	ItemOut->SetupAttachment(Root);
	ItemOut->Direction = EJ3PortDirection::Out;
	ItemOut->SetRelativeLocation(FVector(100.f, 0.f, 50.f));
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
