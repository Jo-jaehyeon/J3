// Copyright (c) ACCENT. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Building/J3BuildingTypes.h"
#include "J3BuildingSubsystem.generated.h"

class AJ3BuildingBase;
class AJ3Character;


// 건물 목록과 공간 인덱스, 설치 판정 (Building_Placement 2.2, 4절).
// 판정은 서버가 확정하고, 클라이언트는 같은 함수로 미리보기 색과 사유를 보여 준다.
UCLASS()
class J3_API UJ3BuildingSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	void RegisterBuilding(AJ3BuildingBase* Building);
	void UnregisterBuilding(AJ3BuildingBase* Building);


	// 이 위치·방향에 이 건물을 지을 수 있는지. Location은 조준한 지점(바닥 근처), 실제 높이는 판정이 찾는다.
	// IgnoreActor: 미리보기 모형처럼 판정에서 빼야 할 액터
	FJ3PlacementCheck CheckPlacement(FName BuildingId, const FVector& Location, float YawDeg, const AJ3Character* Builder, const AActor* IgnoreActor = nullptr) const;

	int32 CountBuildings(FName BuildingId) const;

	int32 CountAllBuildings() const
	{
		return Buildings.Num();
	}

	// 상자 안의 건물 (포트 스냅·철거 대상 찾기)
	TArray<AJ3BuildingBase*> FindBuildingsInBox(const FBox& Box) const;

	// 건물 클래스의 크기 (원점 = 바닥 가운데 기준 반 크기)
	FVector GetFootprintHalfExtent(TSubclassOf<AJ3BuildingBase> BuildingClass) const;

private:
	FIntPoint GetCell(const FVector& Location) const;

	// 서버·클라이언트 각자 가진 건물 목록과 격자 칸 인덱스 (플레이어에게 보이지 않는 그리드 해시)
	TArray<TWeakObjectPtr<AJ3BuildingBase>> Buildings;
	TMap<FIntPoint, TArray<TWeakObjectPtr<AJ3BuildingBase>>> Cells;
	TMap<TWeakObjectPtr<AJ3BuildingBase>, FIntPoint> CellOf;

	float CellSizeCm = 800.f;
};
