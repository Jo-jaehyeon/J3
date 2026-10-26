// Copyright (c) ACCENT. All Rights Reserved.

#include "Building/J3BuildingSubsystem.h"

#include "Building/J3BuildingBase.h"
#include "Core/J3GameState.h"
#include "Core/J3PlayerState.h"
#include "Data/J3DataSubsystem.h"
#include "Engine/World.h"
#include "Inventory/J3InventoryComponent.h"
#include "Inventory/J3InventoryLogic.h"
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


FJ3PlacementCheck UJ3BuildingSubsystem::CheckPlacement(FName BuildingId, const FVector& Location, float YawDeg, const AJ3Character* Builder, const AActor* IgnoreActor) const
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


	// 바닥 찾기: 지형이나 다른 건물의 윗면. 다른 조건이 실패해도 미리보기를 바닥에 붙이려고 먼저 위치를 정한다.
	FCollisionQueryParams GroundParams(SCENE_QUERY_STAT(J3BuildGround), true);
	GroundParams.AddIgnoredActor(IgnoreActor);
	GroundParams.AddIgnoredActor(Builder);
	FCollisionObjectQueryParams GroundTypes;
	GroundTypes.AddObjectTypesToQuery(ECC_WorldStatic);
	GroundTypes.AddObjectTypesToQuery(ECC_WorldDynamic);
	FHitResult Ground;
	if (!GetWorld()->LineTraceSingleByObjectType(Ground, Location + FVector(0.f, 0.f, GroundTraceUpCm), Location - FVector(0.f, 0.f, GroundTraceDownCm), GroundTypes, GroundParams))
	{
		Check.Result = EJ3PlacementResult::NoGround;
		return Check;
	}
	const AJ3BuildingBase* GroundBuilding = Cast<AJ3BuildingBase>(Ground.GetActor());
	if (GroundBuilding != nullptr && !GroundBuilding->CanSupportBuildingsOnTop())
	{
		Check.Result = EJ3PlacementResult::Overlapping;
		return Check;
	}
	const FQuat Rotation = FRotator(0.f, J3Building::QuantizeYaw(YawDeg), 0.f).Quaternion();
	Check.Transform = FTransform(Rotation, J3Building::QuantizeLocation(Ground.ImpactPoint));


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
	if (FVector::Dist(Builder->GetActorLocation(), Location) > Data->GetConfigFloat("build_range_cm", 1500.f))
	{
		Check.Result = EJ3PlacementResult::TooFar;
		return Check;
	}


	// 경사: 지형이든 건물 윗면이든 같은 기준 (25°, Building_Placement 1절)
	const float SlopeDeg = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Ground.ImpactNormal.Z, -1.f, 1.f)));
	if (SlopeDeg > Data->GetConfigFloat("max_slope_deg", 25.f))
	{
		Check.Result = EJ3PlacementResult::TooSteep;
		return Check;
	}


	// 겹침: 바닥(지형·받치는 건물)은 빼고, 다른 물체(건물, 자원 노드, 플레이어)와 겹치면 불가
	const FVector HalfExtent = GetFootprintHalfExtent(const_cast<UClass*>(BuildingClass)) - FVector(OverlapMarginCm);
	const FVector Center = Check.Transform.GetLocation() + Rotation.RotateVector(FVector(0.f, 0.f, HalfExtent.Z + OverlapMarginCm * 2.f));
	FCollisionQueryParams OverlapParams(SCENE_QUERY_STAT(J3BuildOverlap), false);
	OverlapParams.AddIgnoredActor(Ground.GetActor());
	OverlapParams.AddIgnoredActor(IgnoreActor);
	FCollisionObjectQueryParams OverlapTypes = GroundTypes;
	OverlapTypes.AddObjectTypesToQuery(ECC_Pawn);
	if (GetWorld()->OverlapAnyTestByObjectType(Center, Rotation, OverlapTypes, FCollisionShape::MakeBox(HalfExtent.ComponentMax(FVector(1.f))), OverlapParams))
	{
		Check.Result = EJ3PlacementResult::Overlapping;
		return Check;
	}
	Check.Result = EJ3PlacementResult::Ok;
	return Check;
}
