// Copyright (c) ACCENT. All Rights Reserved.

#include "Building/J3BuildingSubsystem.h"

#include "Building/J3BuildingBase.h"
#include "Core/J3GameState.h"
#include "Core/J3PlayerState.h"
#include "Data/J3DataSubsystem.h"
#include "Engine/World.h"
#include "Inventory/J3InventoryComponent.h"
#include "Inventory/J3InventoryLogic.h"
#include "Engine/OverlapResult.h"
#include "LandscapeProxy.h"
#include "Player/J3Character.h"


namespace J3BuildingSubsystemPrivate
{
	// 지면 찾기: 조준점 위아래로 이만큼 훑는다 (건물 위에도 놓을 수 있어 위쪽도 본다)
	constexpr float GroundTraceUpCm = 200.f;
	constexpr float GroundTraceDownCm = 500.f;

	// 겹침 판정 여유: 바닥에 닿은 면이 겹침으로 잡히지 않도록 살짝 띄우고 줄인다
	constexpr float OverlapMarginCm = 2.f;
}


void UJ3BuildingSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	CellSizeCm = FMath::Max(100.f, UJ3DataSubsystem::Get()->GetConfigFloat("build_index_cell_cm", CellSizeCm));
}


FIntPoint UJ3BuildingSubsystem::GetCell(const FVector& Location) const
{
	return FIntPoint(FMath::FloorToInt(Location.X / CellSizeCm), FMath::FloorToInt(Location.Y / CellSizeCm));
}


void UJ3BuildingSubsystem::RegisterBuilding(AJ3BuildingBase* Building)
{
	const FIntPoint Cell = GetCell(Building->GetActorLocation());
	Buildings.AddUnique(Building);
	Cells.FindOrAdd(Cell).AddUnique(Building);
	CellOf.Add(Building, Cell);
}


void UJ3BuildingSubsystem::UnregisterBuilding(AJ3BuildingBase* Building)
{
	Buildings.Remove(Building);
	FIntPoint Cell;
	if (CellOf.RemoveAndCopyValue(Building, Cell))
	{
		if (TArray<TWeakObjectPtr<AJ3BuildingBase>>* InCell = Cells.Find(Cell))
		{
			InCell->Remove(Building);
		}
	}
}


int32 UJ3BuildingSubsystem::CountBuildings(FName BuildingId) const
{
	int32 Count = 0;
	for (const TWeakObjectPtr<AJ3BuildingBase>& Building : Buildings)
	{
		Count += Building.IsValid() && Building->GetBuildingId() == BuildingId ? 1 : 0;
	}
	return Count;
}


TArray<AJ3BuildingBase*> UJ3BuildingSubsystem::FindBuildingsInBox(const FBox& Box) const
{
	// 상자가 걸치는 칸만 본다 (큰 건물이 칸 경계를 넘을 수 있어 한 칸 넓혀 본다)
	TArray<AJ3BuildingBase*> Result;
	const FIntPoint Min = GetCell(Box.Min) - FIntPoint(1, 1);
	const FIntPoint Max = GetCell(Box.Max) + FIntPoint(1, 1);
	for (int32 X = Min.X; X <= Max.X; ++X)
	{
		for (int32 Y = Min.Y; Y <= Max.Y; ++Y)
		{
			const TArray<TWeakObjectPtr<AJ3BuildingBase>>* InCell = Cells.Find(FIntPoint(X, Y));
			if (InCell == nullptr)
			{
				continue;
			}
			for (const TWeakObjectPtr<AJ3BuildingBase>& Building : *InCell)
			{
				if (Building.IsValid() && Box.IsInsideOrOn(Building->GetActorLocation()))
				{
					Result.Add(Building.Get());
				}
			}
		}
	}
	return Result;
}


FVector UJ3BuildingSubsystem::GetFootprintHalfExtent(TSubclassOf<AJ3BuildingBase> BuildingClass) const
{
	const AJ3BuildingBase* Default = BuildingClass != nullptr ? BuildingClass->GetDefaultObject<AJ3BuildingBase>() : nullptr;
	return Default != nullptr ? Default->GetFootprintHalfExtent() : FVector(50.f);
}


bool UJ3BuildingSubsystem::TraceGround(const FVector& Location, const AActor* Builder, const AActor* IgnoreActor, FHitResult& OutHit) const
{
	using namespace J3BuildingSubsystemPrivate;

	// 지형이나 다른 건물의 윗면 (조준점 위아래)
	FCollisionQueryParams Params(SCENE_QUERY_STAT(J3BuildGround), true);
	Params.AddIgnoredActor(IgnoreActor);
	Params.AddIgnoredActor(Builder);
	FCollisionObjectQueryParams Types;
	Types.AddObjectTypesToQuery(ECC_WorldStatic);
	Types.AddObjectTypesToQuery(ECC_WorldDynamic);
	return GetWorld()->LineTraceSingleByObjectType(OutHit, Location + FVector(0.f, 0.f, GroundTraceUpCm), Location - FVector(0.f, 0.f, GroundTraceDownCm), Types, Params);
}


bool UJ3BuildingSubsystem::TrySnapToNeighbor(FName BuildingId, const FVector& HalfExtent, const FVector& Location, float YawDeg, FTransform& OutTransform) const
{
	// 근처의 같은 구조물만 (토대끼리 바닥을 이어 붙인다)
	const UJ3DataSubsystem* Data = UJ3DataSubsystem::Get();
	const float SnapRadiusCm = Data->GetConfigFloat("build_snap_radius_cm", 250.f);
	const float SearchCm = HalfExtent.GetMax() * 2.f + SnapRadiusCm;
	const TArray<AJ3BuildingBase*> Nearby = FindBuildingsInBox(FBox(Location - FVector(SearchCm), Location + FVector(SearchCm)));


	// 이웃의 네 옆 칸 중 조준점에 가장 가까운 빈 칸
	float BestDistance = SnapRadiusCm;
	bool bFound = false;
	for (const AJ3BuildingBase* Neighbor : Nearby)
	{
		if (Neighbor->GetBuildingId() != BuildingId || !Neighbor->IsA<AJ3StructureBuilding>())
		{
			continue;
		}
		const FTransform NeighborTransform = Neighbor->GetActorTransform();
		const FVector Offsets[4] = { FVector(HalfExtent.X * 2.f, 0.f, 0.f), FVector(-HalfExtent.X * 2.f, 0.f, 0.f), FVector(0.f, HalfExtent.Y * 2.f, 0.f), FVector(0.f, -HalfExtent.Y * 2.f, 0.f) };
		for (const FVector& Offset : Offsets)
		{
			const FVector Candidate = NeighborTransform.TransformPosition(Offset);
			const float Distance = FVector::Dist2D(Candidate, Location);
			const bool bOccupied = FindBuildingsInBox(FBox(Candidate - FVector(10.f), Candidate + FVector(10.f))).Num() > 0;
			if (Distance < BestDistance && !bOccupied)
			{
				// 방향은 이웃 기준 90° 단위로 (사용자가 돌린 각도에 가장 가까운 것)
				const float NeighborYaw = NeighborTransform.Rotator().Yaw;
				const float RelativeYaw = FMath::RoundToFloat(FRotator::NormalizeAxis(YawDeg - NeighborYaw) / 90.f) * 90.f;
				OutTransform = FTransform(FRotator(0.f, NeighborYaw + RelativeYaw, 0.f), Candidate);
				BestDistance = Distance;
				bFound = true;
			}
		}
	}
	return bFound;
}


bool UJ3BuildingSubsystem::ResolveTransform(FName BuildingId, const FVector& HalfExtent, const FVector& Location, float YawDeg, bool bGrid, const AActor* Builder, const AActor* IgnoreActor, FTransform& OutTransform, FVector& OutGroundNormal, const AActor*& OutGroundActor, bool& bOutSupported) const
{
	// 1) 보조 그리드가 켜져 있고 같은 토대 옆이면 거기에 붙인다 (높이도 이웃과 같다). 끄면 완전 자유 배치 (Building_Placement 2.1)
	bOutSupported = false;
	if (bGrid && TrySnapToNeighbor(BuildingId, HalfExtent, Location, YawDeg, OutTransform))
	{
		bOutSupported = true;
		OutGroundNormal = FVector::UpVector;
		OutGroundActor = nullptr;
		OutTransform.SetLocation(J3Building::QuantizeLocation(OutTransform.GetLocation()));
		OutTransform.SetRotation(FRotator(0.f, J3Building::QuantizeYaw(OutTransform.Rotator().Yaw), 0.f).Quaternion());
		return true;
	}


	// 2) 바닥 찾기
	FHitResult Ground;
	if (!TraceGround(Location, Builder, IgnoreActor, Ground))
	{
		return false;
	}
	FVector Snapped = Ground.ImpactPoint;
	float SnappedYaw = YawDeg;


	// 3) 보조 그리드: 토대 위면 그 토대 기준 격자, 아니면 월드 격자. 격자에 맞춘 자리의 바닥을 다시 찾는다.
	if (bGrid)
	{
		const float GridCm = FMath::Max(1.f, UJ3DataSubsystem::Get()->GetConfigFloat("build_grid_cm", 100.f));
		const AJ3BuildingBase* Base = Cast<AJ3BuildingBase>(Ground.GetActor());
		const FTransform GridFrame = Base != nullptr ? Base->GetActorTransform() : FTransform::Identity;
		const FVector Local = GridFrame.InverseTransformPosition(Ground.ImpactPoint);
		const FVector LocalSnapped(FMath::GridSnap(Local.X, GridCm), FMath::GridSnap(Local.Y, GridCm), Local.Z);
		Snapped = GridFrame.TransformPosition(LocalSnapped);
		SnappedYaw = GridFrame.Rotator().Yaw + FMath::GridSnap(FRotator::NormalizeAxis(YawDeg - GridFrame.Rotator().Yaw), UJ3DataSubsystem::Get()->GetConfigFloat("build_rotate_step_deg", 15.f));
		if (!TraceGround(Snapped, Builder, IgnoreActor, Ground))
		{
			return false;
		}
		Snapped = Ground.ImpactPoint;
	}
	OutGroundNormal = Ground.ImpactNormal;
	OutGroundActor = Ground.GetActor();
	OutTransform = FTransform(FRotator(0.f, J3Building::QuantizeYaw(SnappedYaw), 0.f), J3Building::QuantizeLocation(Snapped));
	return true;
}


FJ3PlacementCheck UJ3BuildingSubsystem::CheckPlacement(FName BuildingId, const FVector& Location, float YawDeg, bool bGrid, const AJ3Character* Builder, const AActor* IgnoreActor) const
{
	using namespace J3BuildingSubsystemPrivate;

	FJ3PlacementCheck Check;
	const UJ3DataSubsystem* Data = UJ3DataSubsystem::Get();
	const FJ3BuildingDef* Def = Data->GetBuilding(BuildingId);
	const UClass* BuildingClass = Def != nullptr ? Def->BuildingClass.TryLoadClass<AJ3BuildingBase>() : nullptr;
	if (BuildingClass == nullptr || Builder == nullptr)
	{
		Check.Result = EJ3PlacementResult::UnknownBuilding;
		return Check;
	}


	// 위치 정하기. 다른 조건이 실패해도 미리보기를 바닥에 붙이려고 먼저 한다.
	const FVector HalfExtent = GetFootprintHalfExtent(const_cast<UClass*>(BuildingClass));
	FVector GroundNormal;
	const AActor* GroundActor = nullptr;
	bool bSupported = false;
	if (!ResolveTransform(BuildingId, HalfExtent, Location, YawDeg, bGrid, Builder, IgnoreActor, Check.Transform, GroundNormal, GroundActor, bSupported))
	{
		Check.Result = EJ3PlacementResult::NoGround;
		return Check;
	}
	const AJ3BuildingBase* GroundBuilding = Cast<AJ3BuildingBase>(GroundActor);
	if (GroundBuilding != nullptr && !GroundBuilding->CanSupportBuildingsOnTop())
	{
		Check.Result = EJ3PlacementResult::Overlapping;
		return Check;
	}


	// 해방·개수 제한·규모 상한 (팀 상태)
	const AJ3GameState* GameState = GetWorld()->GetGameState<AJ3GameState>();
	if (GameState == nullptr || !GameState->IsUnlocked(Def->Unlock))
	{
		Check.Result = EJ3PlacementResult::NotUnlocked;
		return Check;
	}
	if (Def->Limit > 0 && CountBuildings(BuildingId) >= Def->Limit)
	{
		Check.Result = EJ3PlacementResult::AtLimit;
		return Check;
	}
	if (CountAllBuildings() >= Data->GetConfigInt("max_buildings", 2000))
	{
		Check.Result = EJ3PlacementResult::AtScaleCap;
		return Check;
	}


	// 재료 (건설하는 사람의 인벤토리)
	const AJ3PlayerState* PlayerState = Builder->GetPlayerState<AJ3PlayerState>();
	if (PlayerState == nullptr || !J3InventoryLogic::HasAll(PlayerState->GetInventory()->GetSlots(), Def->BuildInputs))
	{
		Check.Result = EJ3PlacementResult::MissingMaterials;
		return Check;
	}


	// 거리 (몸에서 설치 지점까지)
	if (FVector::Dist(Builder->GetActorLocation(), Check.Transform.GetLocation()) > Data->GetConfigFloat("build_range_cm", 1500.f))
	{
		Check.Result = EJ3PlacementResult::TooFar;
		return Check;
	}


	// 경사: 지형이든 건물 윗면이든 같은 기준 (25°). 이웃 토대에 붙은 토대는 지형과 상관없다 (토대가 경사를 극복하는 수단)
	const float SlopeDeg = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(GroundNormal.Z, -1.f, 1.f)));
	if (!bSupported && SlopeDeg > Data->GetConfigFloat("max_slope_deg", 25.f))
	{
		Check.Result = EJ3PlacementResult::TooSteep;
		return Check;
	}


	// 겹침: 지형(Landscape)과 받치는 바닥은 빼고, 다른 물체(건물, 자원 노드, 플레이어, 배치 물건)와 겹치면 불가
	const FQuat Rotation = Check.Transform.GetRotation();
	const FVector BoxHalf = (HalfExtent - FVector(OverlapMarginCm)).ComponentMax(FVector(1.f));
	const FVector Center = Check.Transform.GetLocation() + Rotation.RotateVector(FVector(0.f, 0.f, BoxHalf.Z + OverlapMarginCm * 2.f));
	FCollisionQueryParams OverlapParams(SCENE_QUERY_STAT(J3BuildOverlap), false);
	OverlapParams.AddIgnoredActor(GroundActor);
	OverlapParams.AddIgnoredActor(IgnoreActor);
	FCollisionObjectQueryParams OverlapTypes;
	OverlapTypes.AddObjectTypesToQuery(ECC_WorldStatic);
	OverlapTypes.AddObjectTypesToQuery(ECC_WorldDynamic);
	OverlapTypes.AddObjectTypesToQuery(ECC_Pawn);
	TArray<FOverlapResult> Overlaps;
	GetWorld()->OverlapMultiByObjectType(Overlaps, Center, Rotation, OverlapTypes, FCollisionShape::MakeBox(BoxHalf), OverlapParams);
	for (const FOverlapResult& Overlap : Overlaps)
	{
		if (Overlap.GetActor() != nullptr && !Overlap.GetActor()->IsA<ALandscapeProxy>())
		{
			Check.Result = EJ3PlacementResult::Overlapping;
			return Check;
		}
	}
	Check.Result = EJ3PlacementResult::Ok;
	return Check;
}
