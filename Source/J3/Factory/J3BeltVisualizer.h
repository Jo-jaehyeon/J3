// Copyright (c) ACCENT. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "J3BeltVisualizer.generated.h"

class UInstancedStaticMeshComponent;


// 공장 시뮬레이션의 컨베이어와 아이템을 인스턴스 메시로 그린다 (각 PC에서 따로, 복제하지 않음).
// 아이템 수천 개를 Actor 없이 그리기 위해 메시 인스턴스 한 묶음으로 처리한다.
// 메시는 블루프린트(BP_J3BeltVisualizer)에서 지정한다. TODO(아트): 아이템 종류별 메시
UCLASS()
class J3_API AJ3BeltVisualizer : public AActor
{
	GENERATED_BODY()

public:
	AJ3BeltVisualizer();

	virtual void Tick(float DeltaSeconds) override;

protected:
	UPROPERTY(VisibleAnywhere, Category = "J3|Visual")
	TObjectPtr<UInstancedStaticMeshComponent> BeltInstances;

	UPROPERTY(VisibleAnywhere, Category = "J3|Visual")
	TObjectPtr<UInstancedStaticMeshComponent> ItemInstances;

	// 컨베이어 메시를 구간 길이에 맞춰 늘릴 때 쓰는 원래 크기 (cm). 기본 큐브는 100.
	UPROPERTY(EditDefaultsOnly, Category = "J3|Visual")
	float BeltMeshLengthCm = 100.f;

	UPROPERTY(EditDefaultsOnly, Category = "J3|Visual")
	FVector BeltScale = FVector(1.f, 0.6f, 0.1f);

	UPROPERTY(EditDefaultsOnly, Category = "J3|Visual")
	FVector ItemScale = FVector(0.15f);

	// 컨베이어 윗면에서 아이템 중심까지 높이
	UPROPERTY(EditDefaultsOnly, Category = "J3|Visual")
	float ItemHeightCm = 12.f;

private:
	void RebuildBelts(const class UJ3FactorySimSubsystem& Simulation);
	void UpdateItems(const class UJ3FactorySimSubsystem& Simulation);

	int32 BuiltTopologyVersion = INDEX_NONE;

	// 구간별 길이표 (곡선 위 아이템 위치 계산용, 구간이 바뀔 때만 다시 만든다)
	struct FArcTable
	{
		TArray<FVector> Points;
		TArray<float> Distances;
	};
	TMap<int32, FArcTable> ArcTables;

	// 길이표 점 간격 (cm)
	UPROPERTY(EditDefaultsOnly, Category = "J3|Visual")
	float ArcSampleCm = 25.f;

	// 매 프레임 재사용 (할당 줄이기)
	TArray<FTransform> ItemTransforms;
};
