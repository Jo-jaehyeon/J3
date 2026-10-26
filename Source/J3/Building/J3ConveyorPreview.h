// Copyright (c) ACCENT. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "J3ConveyorPreview.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;


// 컨베이어 설치 청사진 (이 PC에서만, 충돌 없음). 시작점 전에는 붙을 점에 작은 표시, 시작점 뒤에는 곡선을 조각으로 그려 파랑/빨강으로 칠한다.
UCLASS(NotPlaceable, Transient)
class J3_API AJ3ConveyorPreview : public AActor
{
	GENERATED_BODY()

public:
	AJ3ConveyorPreview();

	// 포트 자리 표시 (파랑: 이을 수 있음, 빨강: 못 이음)
	void SetMarkers(const TArray<FVector>& Locations, bool bValid);

	void SetPath(const struct FJ3ConveyorPath& Path, bool bValid);

protected:
	UPROPERTY(VisibleAnywhere, Category = "J3|Building")
	TObjectPtr<UInstancedStaticMeshComponent> Pieces;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> Material;

	UPROPERTY(EditDefaultsOnly, Category = "J3|Style")
	FLinearColor ValidColor = FLinearColor(0.1f, 0.4f, 1.f);

	UPROPERTY(EditDefaultsOnly, Category = "J3|Style")
	FLinearColor InvalidColor = FLinearColor(1.f, 0.15f, 0.1f);

	// 표시 크기 (기본 큐브 100cm 기준 배율)
	UPROPERTY(EditDefaultsOnly, Category = "J3|Style")
	FVector MarkerScale = FVector(0.3f);

	// 곡선 조각 길이와 단면 배율 (벨트 표시와 비슷하게)
	UPROPERTY(EditDefaultsOnly, Category = "J3|Style")
	float PieceLengthCm = 50.f;

	UPROPERTY(EditDefaultsOnly, Category = "J3|Style")
	FVector PieceScale = FVector(1.f, 0.6f, 0.1f);

private:
	void SetColor(bool bValid);
};
