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


	// 이 위치·방향에 이 건물을 지을 수 있는지. Location은 조준한 지점(바닥 근처), 실제 위치는 판정이 정한다:
	// 보조 그리드가 켜져 있으면 같은 토대 옆 가장자리에 붙이거나(스냅) 격자에 맞추고, 끄면 조준한 바닥에 그대로 놓는다.
	// IgnoreActor: 미리보기 모형처럼 판정에서 빼야 할 액터
	FJ3PlacementCheck CheckPlacement(FName BuildingId, const FVector& Location, float YawDeg, bool bGrid, const AJ3Character* Builder, const AActor* IgnoreActor = nullptr) const;

	int32 CountBuildings(FName BuildingId) const;

	int32 CountAllBuildings() const
	{
		return Buildings.Num();
	}

	// 모든 건물 (검증·세이브용)
	TArray<AJ3BuildingBase*> GetAllBuildings() const
	{
		TArray<AJ3BuildingBase*> Result;
		for (const TWeakObjectPtr<AJ3BuildingBase>& Building : Buildings)
		{
			if (Building.IsValid())
			{
				Result.Add(Building.Get());
			}
		}
		return Result;
	}

	// 상자 안의 건물 (포트 스냅·철거 대상 찾기)
	TArray<AJ3BuildingBase*> FindBuildingsInBox(const FBox& Box) const;

	// 건물 클래스의 크기 (원점 = 바닥 가운데 기준 반 크기)
	FVector GetFootprintHalfExtent(TSubclassOf<AJ3BuildingBase> BuildingClass) const;

private:
	FIntPoint GetCell(const FVector& Location) const;

	// 최종 위치 정하기. 바닥이 없으면 false. bOutSupported: 이웃에 붙어 지형 경사와 상관없이 놓였다.
	// OutAnchor: 자원 위에만 짓는 건물이 올라간 자원 노드 (못 찾으면 null)
	bool ResolveTransform(FName BuildingId, const FVector& HalfExtent, const FVector& Location, float YawDeg, bool bGrid, const AActor* Builder, const AActor* IgnoreActor, FTransform& OutTransform, FVector& OutGroundNormal, const AActor*& OutGroundActor, bool& bOutSupported, const AActor*& OutAnchor) const;

	// 조준점 근처의 허용된 자원 노드 중 가장 가까운 것 (Building_Placement: 채굴기는 자원 위에만)
	const AActor* FindAnchorNode(const TArray<FName>& AllowedNodes, const FVector& Location) const;

	// 보조 그리드가 켜져 있을 때 같은 건물의 옆 칸(앞뒤좌우)에 붙인다 (Building_Placement 2.1 스냅)
	bool TrySnapToNeighbor(FName BuildingId, const FVector& HalfExtent, const FVector& Location, float YawDeg, FTransform& OutTransform) const;

	// 근처 건물의 맞는 포트와 마주 보게 놓는다 (Building_Placement 2.1, 보조 그리드와 상관없이 항상). 위치(XY)와 방향만 정한다
	bool TrySnapToPort(TSubclassOf<AJ3BuildingBase> BuildingClass, const FVector& HalfExtent, const FVector& Location, FVector& OutLocation, float& OutYawDeg) const;

	bool TraceGround(const FVector& Location, const AActor* Builder, const AActor* IgnoreActor, FHitResult& OutHit) const;

	// 서버·클라이언트 각자 가진 건물 목록과 격자 칸 인덱스 (플레이어에게 보이지 않는 그리드 해시)
	TArray<TWeakObjectPtr<AJ3BuildingBase>> Buildings;
	TMap<FIntPoint, TArray<TWeakObjectPtr<AJ3BuildingBase>>> Cells;
	TMap<TWeakObjectPtr<AJ3BuildingBase>, FIntPoint> CellOf;

	float CellSizeCm = 800.f;
};
