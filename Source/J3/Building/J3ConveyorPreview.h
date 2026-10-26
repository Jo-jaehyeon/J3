// Copyright (c) ACCENT. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "J3ConveyorPreview.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
struct FJ3ConveyorPath;


// 컨베이어 설치 미리보기 (이 PC에서만, 충돌 없음). 곡선을 짧은 상자 조각으로 그리고 파랑/빨강으로 칠한다.
UCLASS(NotPlaceable, Transient)
class J3_API AJ3ConveyorPreview : public AActor
{
	GENERATED_BODY()

public:
	AJ3ConveyorPreview();

	void SetPath(const FJ3ConveyorPath& Path, bool bValid);

	// 시작점을 고르기 전: 붙을 점에 작은 표시만
	void SetMarker(const FVector& Location, const FVector& Direction, bool bValid);

protected:
	UPROPERTY(VisibleAnywhere, Category = "J3|Building")
	TObjectPtr<UInstancedStaticMeshComponent> Pieces;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> Material;

	UPROPERTY(EditDefaultsOnly, Category = "J3|Style")
	FLinearColor ValidColor = FLinearColor(0.1f, 0.4f, 1.f);

	UPROPERTY(EditDefaultsOnly, Category = "J3|Style")
	FLinearColor InvalidColor = FLinearColor(1.f, 0.15f, 0.1f);

	// 조각 길이와 단면 배율 (기본 큐브 100cm 기준, 벨트 표시와 비슷하게)
	UPROPERTY(EditDefaultsOnly, Category = "J3|Style")
	float PieceLengthCm = 50.f;

	UPROPERTY(EditDefaultsOnly, Category = "J3|Style")
	FVector PieceScale = FVector(1.f, 0.6f, 0.1f);

private:
	void SetColor(bool bValid);
};
