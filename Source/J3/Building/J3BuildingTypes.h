// Copyright (c) ACCENT. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "J3BuildingTypes.generated.h"


// 설치 판정 결과 (Building_Placement 2.2). 실패하면 미리보기가 빨간색이 되고 사유 문장을 보여 준다.
UENUM(BlueprintType)
enum class EJ3PlacementResult : uint8
{
	Ok,
	UnknownBuilding,
	NotUnlocked,
	MissingMaterials,
	AtLimit,
	AtScaleCap,
	TooFar,
	NoGround,
	TooSteep,
	Overlapping,
};


// 설치 판정 결과와 확정 위치
USTRUCT(BlueprintType)
struct FJ3PlacementCheck
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "J3|Building")
	EJ3PlacementResult Result = EJ3PlacementResult::UnknownBuilding;

	// 지면·건물 위에 놓인 최종 위치 (원점 = 바닥 가운데)
	UPROPERTY(BlueprintReadOnly, Category = "J3|Building")
	FTransform Transform;

	bool IsOk() const
	{
		return Result == EJ3PlacementResult::Ok;
	}
};


namespace J3Building
{
	// 사유 문장 (미리보기·안내용)
	J3_API FText GetResultText(EJ3PlacementResult Result);

	// 위치·회전 양자화: 서버·클라이언트·세이브가 같은 값을 쓰도록 위치 1cm, 회전 1° 단위로 맞춘다 (Building_Placement 4절)
	J3_API FVector QuantizeLocation(const FVector& Location);
	J3_API float QuantizeYaw(float YawDeg);
}
