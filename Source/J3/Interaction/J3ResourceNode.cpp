// Copyright (c) ACCENT. All Rights Reserved.

#include "Interaction/J3ResourceNode.h"

#include "Components/StaticMeshComponent.h"
#include "Data/J3DataSubsystem.h"
#include "Net/UnrealNetwork.h"
#include "Player/J3Character.h"
#include "Player/J3HarvestComponent.h"


AJ3ResourceNode::AJ3ResourceNode()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;


	// 조준 판정과 이동 충돌 모두 막는다 (바위·나무는 지나갈 수 없다)
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	RootComponent = Mesh;
}


void AJ3ResourceNode::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AJ3ResourceNode, NodeId);
}


void AJ3ResourceNode::BeginPlay()
{
	Super::BeginPlay();

	// 노드는 무한형이라 상태가 바뀌지 않는다. 처음 한 번 보내고 나면 복제를 쉬게 해 비용을 없앤다.
	if (HasAuthority())
	{
		SetNetDormancy(DORM_DormantAll);
	}
	RefreshMesh();
}


void AJ3ResourceNode::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshMesh();
}


void AJ3ResourceNode::SetNodeId(FName InNodeId)
{
	NodeId = InNodeId;
	RefreshMesh();
}


void AJ3ResourceNode::OnRep_NodeId()
{
	RefreshMesh();
}


void AJ3ResourceNode::RefreshMesh()
{
	if (const TObjectPtr<UStaticMesh>* Found = NodeMeshes.Find(NodeId))
	{
		Mesh->SetStaticMesh(*Found);
	}
	if (const FVector* Scale = NodeScales.Find(NodeId))
	{
		Mesh->SetRelativeScale3D(*Scale);
	}
}


const FJ3ResourceNodeDef* AJ3ResourceNode::GetDef() const
{
	return UJ3DataSubsystem::Get()->GetResourceNode(NodeId);
}


bool AJ3ResourceNode::CanInteract(const AJ3Character* Interactor) const
{
	// 채취 가능 여부(필요 도구, 사망, 이미 채취 중)는 채취 컴포넌트가 판단한다
	const UJ3HarvestComponent* Harvest = Interactor != nullptr ? Interactor->GetHarvest() : nullptr;
	return GetDef() != nullptr && Harvest != nullptr && Harvest->CanHarvest(*this);
}


void AJ3ResourceNode::Interact(AJ3Character* Interactor)
{
	check(HasAuthority());
	Interactor->GetHarvest()->StartHarvest(*this);
}
