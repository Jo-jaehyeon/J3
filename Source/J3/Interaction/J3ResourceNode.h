// Copyright (c) ACCENT. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/J3Interactable.h"
#include "J3ResourceNode.generated.h"

class UStaticMesh;
class UStaticMeshComponent;
struct FJ3ResourceNodeDef;


// 월드의 자원 노드 (바위, 덤불, 광맥 등). 무한형이라 사라지지 않는다 (Resources_And_Tiers 1절).
// G키로 채취를 시작하면 플레이어의 채취 컴포넌트가 진행도를 채우고, 다 차면 아이템을 준다.
// 종류는 resource_nodes.csv의 행이고, 모양은 블루프린트(BP_J3ResourceNode)의 종류별 메시에서 고른다.
UCLASS()
class J3_API AJ3ResourceNode : public AActor, public IJ3Interactable
{
	GENERATED_BODY()

public:
	AJ3ResourceNode();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void OnConstruction(const FTransform& Transform) override;

	virtual bool CanInteract(const AJ3Character* Interactor) const override;
	virtual void Interact(AJ3Character* Interactor) override;

	// 서버: 시드 배치기가 만들 때 종류를 정한다
	void SetNodeId(FName InNodeId);

	FName GetNodeId() const
	{
		return NodeId;
	}

	const FJ3ResourceNodeDef* GetDef() const;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnRep_NodeId();

	void RefreshMesh();

	UPROPERTY(VisibleAnywhere, Category = "J3|Resource")
	TObjectPtr<UStaticMeshComponent> Mesh;

	// resource_nodes.csv의 id
	UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_NodeId, Category = "J3|Resource")
	FName NodeId;

	// 종류별 모양 (아트). TODO(아트): 실제 바위·나무·광맥 메시
	UPROPERTY(EditDefaultsOnly, Category = "J3|Resource")
	TMap<FName, TObjectPtr<UStaticMesh>> NodeMeshes;

	UPROPERTY(EditDefaultsOnly, Category = "J3|Resource")
	TMap<FName, FVector> NodeScales;
};
