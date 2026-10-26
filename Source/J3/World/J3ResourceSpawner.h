// Copyright (c) ACCENT. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "J3ResourceSpawner.generated.h"

class AJ3ResourceNode;


// 구역 하나의 자원 노드를 월드 시드로 배치한다 (서버에서만, 노드는 복제된다).
// 맵에 구역마다 하나씩 놓고, 범위와 구역 이름(resource_spawns.csv의 zone)을 지정한다.
// 보장 반경의 중심은 시작 지점(PlayerStart)이다. 기지 위치와는 상관없다 (Resources_And_Tiers 1절).
UCLASS()
class J3_API AJ3ResourceSpawner : public AActor
{
	GENERATED_BODY()

public:
	AJ3ResourceSpawner();

protected:
	virtual void BeginPlay() override;

	// resource_spawns.csv의 zone
	UPROPERTY(EditAnywhere, Category = "J3|Resource")
	FName ZoneId = "dev";

	// 이 액터 위치를 중심으로 한 구역의 반 크기 (cm, 맵 배치 값)
	UPROPERTY(EditAnywhere, Category = "J3|Resource")
	FVector2D AreaHalfExtent = FVector2D(4000.f, 4000.f);

	// 만들 노드 클래스 (모양을 지정한 블루프린트)
	UPROPERTY(EditAnywhere, Category = "J3|Resource")
	TSubclassOf<AJ3ResourceNode> NodeClass;

private:
	FVector GetStartLocation() const;
	void SpawnNode(FName NodeId, const FVector2D& Location);

	// 서버: 이 배치 액터가 만든 노드 (지면 찾기에서 서로를 무시하기 위해)
	TArray<AActor*> SpawnedNodes;
};
