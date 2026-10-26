// Copyright (c) ACCENT. All Rights Reserved.

#include "Building/J3BuildingSubsystem.h"

#include "Building/J3BuildingBase.h"
#include "Building/J3ConveyorBuilding.h"
#include "Core/J3GameState.h"
#include "Core/J3PlayerState.h"
#include "Data/J3DataSubsystem.h"
#include "Engine/World.h"
#include "Inventory/J3InventoryComponent.h"
#include "Inventory/J3InventoryLogic.h"
#include "Engine/OverlapResult.h"
#include "Interaction/J3Interactable.h"
#include "Interaction/J3ResourceNode.h"
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
	TArray<FHitResult> Hits;
	GetWorld()->LineTraceMultiByObjectType(Hits, Location + FVector(0.f, 0.f, GroundTraceUpCm), Location - FVector(0.f, 0.f, GroundTraceDownCm), Types, Params);


	// 줍는 아이템·자원 노드 같은 상호작용 물체는 바닥이 아니다 (그 위에 짓지 않고, 겹침 판정에서 걸린다)
	for (const FHitResult& Hit : Hits)
	{
		const AActor* Actor = Hit.GetActor();
		if (Actor != nullptr && !Actor->IsA<AJ3BuildingBase>() && Actor->Implements<UJ3Interactable>())
		{
			continue;
		}
		OutHit = Hit;
		return true;
	}
	return false;
}


bool UJ3BuildingSubsystem::TrySnapToPort(TSubclassOf<AJ3BuildingBase> BuildingClass, const FVector& HalfExtent, const FVector& Location, FVector& OutLocation, float& OutYawDeg) const
{
	// 포트가 없는 건물(토대 등)은 붙지 않는다
	TArray<FJ3PortInfo> MyPorts;
	AJ3BuildingBase::GetClassPorts(BuildingClass, MyPorts);
	if (MyPorts.Num() == 0)
	{
		return false;
	}
	const float SnapRadiusCm = UJ3DataSubsystem::Get()->GetConfigFloat("build_port_snap_radius_cm", 150.f);
	const float SearchCm = HalfExtent.GetMax() * 4.f + SnapRadiusCm;
	const TArray<AJ3BuildingBase*> Nearby = FindBuildingsInBox(FBox(Location - FVector(SearchCm), Location + FVector(SearchCm)));


	// 내 포트 P를 이웃 포트 Q에 맞췄을 때의 건물 원점이 조준점에 가장 가까운 짝
	float BestDistance = SnapRadiusCm;
	bool bFound = false;
	TArray<FJ3PortInfo> TheirPorts;
	for (const AJ3BuildingBase* Neighbor : Nearby)
	{
		TheirPorts.Reset();
		Neighbor->GetPorts(TheirPorts);
		for (const FJ3PortInfo& Their : TheirPorts)
		{
			for (const FJ3PortInfo& Mine : MyPorts)
			{
				if (Mine.Kind != Their.Kind || Mine.Direction == Their.Direction)
				{
					continue;
				}
				// 두 포트가 마주 보게: 내 포트의 바깥 방향 = 이웃 포트 바깥 방향의 반대
				const float Yaw = FRotator::NormalizeAxis(Their.Transform.Rotator().Yaw + 180.f - Mine.Transform.Rotator().Yaw);
				const FVector Offset = FRotator(0.f, Yaw, 0.f).RotateVector(Mine.Transform.GetLocation());
				const FVector Candidate(Their.Transform.GetLocation().X - Offset.X, Their.Transform.GetLocation().Y - Offset.Y, Location.Z);
				const float Distance = FVector::Dist2D(Candidate, Location);
				if (Distance < BestDistance)
				{
					BestDistance = Distance;
					OutLocation = Candidate;
					OutYawDeg = Yaw;
					bFound = true;
				}
			}
		}
	}
	return bFound;
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
		if (Neighbor->GetBuildingId() != BuildingId)
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


const AActor* UJ3BuildingSubsystem::FindAnchorNode(const TArray<FName>& AllowedNodes, const FVector& Location, const AActor* IgnoreBuilding) const
{
	// 조준점 둘레의 자원 노드 (노드는 공간 인덱스에 없어 충돌 검사로 찾는다)
	const float RadiusCm = UJ3DataSubsystem::Get()->GetConfigFloat("build_node_snap_radius_cm", 200.f);
	FCollisionObjectQueryParams Types;
	Types.AddObjectTypesToQuery(ECC_WorldStatic);
	Types.AddObjectTypesToQuery(ECC_WorldDynamic);
	TArray<FOverlapResult> Overlaps;
	GetWorld()->OverlapMultiByObjectType(Overlaps, Location, FQuat::Identity, Types, FCollisionShape::MakeSphere(RadiusCm), FCollisionQueryParams(SCENE_QUERY_STAT(J3BuildAnchor), false));


	// 허용된 종류 중 비어 있는(이미 건물이 올라가 있지 않은) 가장 가까운 것
	const AActor* Best = nullptr;
	float BestDistance = TNumericLimits<float>::Max();
	for (const FOverlapResult& Overlap : Overlaps)
	{
		const AJ3ResourceNode* Node = Cast<AJ3ResourceNode>(Overlap.GetActor());
		const float Distance = Node != nullptr ? FVector::Dist2D(Node->GetActorLocation(), Location) : BestDistance;
		TArray<AJ3BuildingBase*> OnNode = Node != nullptr ? FindBuildingsInBox(FBox(Node->GetActorLocation() - FVector(RadiusCm * 0.5f), Node->GetActorLocation() + FVector(RadiusCm * 0.5f))) : TArray<AJ3BuildingBase*>();
		OnNode.Remove(const_cast<AJ3BuildingBase*>(Cast<AJ3BuildingBase>(IgnoreBuilding)));
		const bool bOccupied = OnNode.Num() > 0;
		if (Node != nullptr && AllowedNodes.Contains(Node->GetNodeId()) && Distance < BestDistance && !bOccupied)
		{
			Best = Node;
			BestDistance = Distance;
		}
	}
	return Best;
}


bool UJ3BuildingSubsystem::ResolveTransform(FName BuildingId, const FVector& HalfExtent, const FVector& Location, float YawDeg, bool bGrid, const AActor* Builder, const AActor* IgnoreActor, FTransform& OutTransform, FVector& OutGroundNormal, const AActor*& OutGroundActor, bool& bOutSupported, const AActor*& OutAnchor) const
{
	// 0) 자원 위에만 짓는 건물(채굴기): 가까운 맞는 자원 노드 자리에, 방향은 사용자가 돌린 대로
	const FJ3BuildingDef* Def = UJ3DataSubsystem::Get()->GetBuilding(BuildingId);
	const TSubclassOf<AJ3BuildingBase> BuildingClass = Def != nullptr ? Def->BuildingClass.TryLoadClass<AJ3BuildingBase>() : nullptr;
	OutAnchor = nullptr;
	bOutSupported = false;
	if (Def != nullptr && Def->PlaceOn.Num() > 0)
	{
		OutAnchor = FindAnchorNode(Def->PlaceOn, Location);
		FHitResult AnchorGround;
		const FVector AnchorLocation = OutAnchor != nullptr ? OutAnchor->GetActorLocation() : Location;
		if (!TraceGround(AnchorLocation, Builder, IgnoreActor, AnchorGround))
		{
			return false;
		}
		OutGroundNormal = AnchorGround.ImpactNormal;
		OutGroundActor = AnchorGround.GetActor();
		OutTransform = FTransform(FRotator(0.f, J3Building::QuantizeYaw(YawDeg), 0.f), J3Building::QuantizeLocation(AnchorGround.ImpactPoint));
		return true;
	}


	// 1) 포트 스냅: 맞는 포트가 가까우면 그 자리·방향으로, 높이는 그 자리의 바닥
	FVector PortLocation;
	float PortYaw = 0.f;
	FHitResult PortGround;
	if (TrySnapToPort(BuildingClass, HalfExtent, Location, PortLocation, PortYaw) && TraceGround(PortLocation, Builder, IgnoreActor, PortGround))
	{
		OutGroundNormal = PortGround.ImpactNormal;
		OutGroundActor = PortGround.GetActor();
		OutTransform = FTransform(FRotator(0.f, J3Building::QuantizeYaw(PortYaw), 0.f), J3Building::QuantizeLocation(PortGround.ImpactPoint));
		return true;
	}


	// 2) 보조 그리드가 켜져 있고 같은 건물 옆이면 거기에 붙인다 (높이도 이웃과 같다). 끄면 완전 자유 배치 (Building_Placement 2.1)
	if (bGrid && TrySnapToNeighbor(BuildingId, HalfExtent, Location, YawDeg, OutTransform))
	{
		bOutSupported = true;
		OutGroundNormal = FVector::UpVector;
		OutGroundActor = nullptr;
		OutTransform.SetLocation(J3Building::QuantizeLocation(OutTransform.GetLocation()));
		OutTransform.SetRotation(FRotator(0.f, J3Building::QuantizeYaw(OutTransform.Rotator().Yaw), 0.f).Quaternion());
		return true;
	}


	// 3) 바닥 찾기
	FHitResult Ground;
	if (!TraceGround(Location, Builder, IgnoreActor, Ground))
	{
		return false;
	}
	FVector Snapped = Ground.ImpactPoint;
	float SnappedYaw = YawDeg;


	// 4) 보조 그리드: 토대 위면 그 토대 기준 격자, 아니면 월드 격자. 격자에 맞춘 자리의 바닥을 다시 찾는다.
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


EJ3PlacementResult UJ3BuildingSubsystem::CheckCommon(FName BuildingId, const FJ3BuildingDef& Def, const AJ3Character* Builder) const
{
	// 해방·개수 제한·규모 상한 (팀 상태)
	const UJ3DataSubsystem* Data = UJ3DataSubsystem::Get();
	const AJ3GameState* GameState = GetWorld()->GetGameState<AJ3GameState>();
	if (GameState == nullptr || !GameState->IsUnlocked(Def.Unlock))
	{
		return EJ3PlacementResult::NotUnlocked;
	}
	if (Def.Limit > 0 && CountBuildings(BuildingId) >= Def.Limit)
	{
		return EJ3PlacementResult::AtLimit;
	}
	if (CountAllBuildings() >= Data->GetConfigInt("max_buildings", 2000))
	{
		return EJ3PlacementResult::AtScaleCap;
	}


	// 재료 (건설하는 사람의 인벤토리)
	const AJ3PlayerState* PlayerState = Builder->GetPlayerState<AJ3PlayerState>();
	if (PlayerState == nullptr || !J3InventoryLogic::HasAll(PlayerState->GetInventory()->GetSlots(), Def.BuildInputs))
	{
		return EJ3PlacementResult::MissingMaterials;
	}
	return EJ3PlacementResult::Ok;
}


EJ3PlacementResult UJ3BuildingSubsystem::ResolveConveyorEnd(bool bStart, const FVector& Aim, const AActor* Builder, FVector& OutLocation, FVector& OutDirection, bool& bOutDirected) const
{
	// 근처 건물 (컨베이어는 시작점에 등록되어 있어, 끝점까지 보려면 최대 길이만큼 넓게 본다)
	const UJ3DataSubsystem* Data = UJ3DataSubsystem::Get();
	const float SnapCm = Data->GetConfigFloat("build_port_snap_radius_cm", 150.f);
	const float SearchCm = Data->GetConfigFloat("conveyor_max_length_cm", 3000.f) + SnapCm;
	const TArray<AJ3BuildingBase*> Nearby = FindBuildingsInBox(FBox(Aim - FVector(SearchCm), Aim + FVector(SearchCm)));


	// 붙을 수 있는 점: 시작은 출구·바닥에서 멈춘 컨베이어 끝, 끝은 입구·바닥에서 시작한 컨베이어 시작.
	// 이미 쓰는 점은 막는다: 다른 컨베이어가 붙은 포트, 그리고 반대쪽 포트에 들어간 컨베이어 끝(입구에 들어간 컨베이어 끝에서 새로 시작할 수 없다)
	TArray<TPair<FVector, FVector>> Candidates;
	TArray<FVector> UsedPoints;
	TArray<FVector> OppositePorts;
	TArray<FJ3ConveyorPath> Conveyors;
	for (const AJ3BuildingBase* Building : Nearby)
	{
		if (const AJ3ConveyorBuilding* Conveyor = Cast<AJ3ConveyorBuilding>(Building))
		{
			Conveyors.Add(Conveyor->GetPath());
			continue;
		}
		if (!Building->CanConnectConveyor())
		{
			continue;
		}
		TArray<FJ3PortInfo> Ports;
		Building->GetPorts(Ports);
		for (const FJ3PortInfo& Port : Ports)
		{
			const bool bWanted = Port.Kind == EJ3PortKind::Item && Port.Direction == (bStart ? EJ3PortDirection::Out : EJ3PortDirection::In);
			if (Port.Kind == EJ3PortKind::Item && !bWanted)
			{
				OppositePorts.Add(Port.Transform.GetLocation());
				continue;
			}
			if (bWanted)
			{
				// 포트의 X축은 바깥을 향한다: 출구는 그 방향으로 나가고, 입구는 반대 방향으로 들어온다
				const FVector Outward = Port.Transform.GetRotation().GetForwardVector();
				Candidates.Emplace(Port.Transform.GetLocation(), bStart ? Outward : -Outward);
			}
		}
	}
	for (const FJ3ConveyorPath& Path : Conveyors)
	{
		const FVector FreeEnd = bStart ? Path.End : Path.Start;
		Candidates.Emplace(FreeEnd, bStart ? Path.EndDirection : Path.StartDirection);
		UsedPoints.Add(bStart ? Path.Start : Path.End);
		if (OppositePorts.ContainsByPredicate([&FreeEnd](const FVector& Point) { return FVector::Dist(Point, FreeEnd) <= 2.f; }))
		{
			UsedPoints.Add(FreeEnd);
		}
	}


	// 조준점에 가장 가까운 점. 이미 쓰이고 있으면 막는다 (한 점에 컨베이어 하나)
	float BestDistance = SnapCm;
	bool bFound = false;
	bool bBestUsed = false;
	for (const TPair<FVector, FVector>& Candidate : Candidates)
	{
		const float Distance = FVector::Dist(Candidate.Key, Aim);
		if (Distance < BestDistance)
		{
			BestDistance = Distance;
			bBestUsed = UsedPoints.ContainsByPredicate([&Candidate](const FVector& Point) { return FVector::Dist(Point, Candidate.Key) <= 2.f; });
			OutLocation = Candidate.Key;
			OutDirection = Candidate.Value.GetSafeNormal();
			bFound = true;
		}
	}
	bOutDirected = bFound && !bBestUsed;
	if (bFound)
	{
		return bBestUsed ? EJ3PlacementResult::PortInUse : EJ3PlacementResult::Ok;
	}


	// 붙을 곳이 없으면 찍은 바닥 위(포트 높이)에서 멈춘다
	FHitResult Ground;
	if (!TraceGround(Aim, Builder, nullptr, Ground))
	{
		return EJ3PlacementResult::NoGround;
	}
	OutLocation = J3Building::QuantizeLocation(Ground.ImpactPoint + FVector(0.f, 0.f, Data->GetConfigFloat("conveyor_height_cm", 50.f)));
	OutDirection = FVector::ForwardVector;
	return EJ3PlacementResult::Ok;
}


FJ3PlacementCheck UJ3BuildingSubsystem::CheckConveyor(FName BuildingId, const FVector& StartAim, const FVector& EndAim, const AJ3Character* Builder, float EndYawOffsetDeg) const
{
	FJ3PlacementCheck Check;
	const UJ3DataSubsystem* Data = UJ3DataSubsystem::Get();
	const FJ3BuildingDef* Def = Data->GetBuilding(BuildingId);
	if (Def == nullptr || Builder == nullptr)
	{
		Check.Result = EJ3PlacementResult::UnknownBuilding;
		return Check;
	}


	// 두 끝 (포트·컨베이어 끝에 붙으면 그 방향, 바닥이면 두 점을 잇는 방향. 바닥 끝은 휠로 돌린 만큼 더 돌린다). 곡선은 저절로 정해진다
	FJ3ConveyorPath& Path = Check.ConveyorPath;
	bool bStartDirected = false;
	bool bEndDirected = false;
	const EJ3PlacementResult StartResult = ResolveConveyorEnd(true, StartAim, Builder, Path.Start, Path.StartDirection, bStartDirected);
	const EJ3PlacementResult EndResult = ResolveConveyorEnd(false, EndAim, Builder, Path.End, Path.EndDirection, bEndDirected);
	if (StartResult != EJ3PlacementResult::Ok || EndResult != EJ3PlacementResult::Ok)
	{
		Check.Result = StartResult != EJ3PlacementResult::Ok ? StartResult : EndResult;
		return Check;
	}
	const FVector Chord = (Path.End - Path.Start).GetSafeNormal();
	Path.StartDirection = bStartDirected ? Path.StartDirection : Chord;
	Path.EndDirection = bEndDirected ? Path.EndDirection : FRotator(0.f, J3Building::QuantizeYaw(EndYawOffsetDeg), 0.f).RotateVector(Chord);
	Check.Transform = FTransform(Path.StartDirection.Rotation(), Path.Start);


	// 공통 (해방·상한·재료)
	Check.Result = CheckCommon(BuildingId, *Def, Builder);
	if (!Check.IsOk())
	{
		return Check;
	}


	// 거리: 두 끝 모두 건설 거리 안
	const float RangeCm = Data->GetConfigFloat("build_range_cm", 1500.f);
	if (FVector::Dist(Builder->GetActorLocation(), Path.Start) > RangeCm || FVector::Dist(Builder->GetActorLocation(), Path.End) > RangeCm)
	{
		Check.Result = EJ3PlacementResult::TooFar;
		return Check;
	}


	// 길이와 꺾임 (곡선 길이로 본다, Building_Placement 3절: 최소 곡률 제한)
	// TODO(기획-제안): 다른 건물과 겹치는 컨베이어를 막을지
	const float Length = Path.ComputeLength();
	if (Length < Data->GetConfigFloat("conveyor_min_length_cm", 50.f))
	{
		Check.Result = EJ3PlacementResult::ConveyorTooShort;
		return Check;
	}
	if (Length > Data->GetConfigFloat("conveyor_max_length_cm", 3000.f))
	{
		Check.Result = EJ3PlacementResult::ConveyorTooLong;
		return Check;
	}
	if (Path.ComputeMinRadius() < Data->GetConfigFloat("conveyor_min_radius_cm", 100.f))
	{
		Check.Result = EJ3PlacementResult::ConveyorTooSharp;
		return Check;
	}
	Check.Result = EJ3PlacementResult::Ok;
	return Check;
}


FJ3PlacementCheck UJ3BuildingSubsystem::CheckPlacement(FName BuildingId, const FVector& Location, float YawDeg, bool bGrid, const AJ3Character* Builder, const AActor* IgnoreActor) const
{
	using namespace J3BuildingSubsystemPrivate;

	FJ3PlacementCheck Check;
	const UJ3DataSubsystem* Data = UJ3DataSubsystem::Get();
	const FJ3BuildingDef* Def = Data->GetBuilding(BuildingId);
	const UClass* BuildingClass = Def != nullptr ? Def->BuildingClass.TryLoadClass<AJ3BuildingBase>() : nullptr;
	if (BuildingClass == nullptr || Builder == nullptr || BuildingClass->IsChildOf<AJ3ConveyorBuilding>())
	{
		// 컨베이어는 CheckConveyor로 판정한다
		Check.Result = EJ3PlacementResult::UnknownBuilding;
		return Check;
	}


	// 위치 정하기. 다른 조건이 실패해도 미리보기를 바닥에 붙이려고 먼저 한다.
	const FVector HalfExtent = GetFootprintHalfExtent(const_cast<UClass*>(BuildingClass));
	FVector GroundNormal;
	const AActor* GroundActor = nullptr;
	bool bSupported = false;
	const AActor* Anchor = nullptr;
	if (!ResolveTransform(BuildingId, HalfExtent, Location, YawDeg, bGrid, Builder, IgnoreActor, Check.Transform, GroundNormal, GroundActor, bSupported, Anchor))
	{
		Check.Result = EJ3PlacementResult::NoGround;
		return Check;
	}
	if (Def->PlaceOn.Num() > 0 && Anchor == nullptr)
	{
		Check.Result = EJ3PlacementResult::NeedsResourceNode;
		return Check;
	}
	const AJ3BuildingBase* GroundBuilding = Cast<AJ3BuildingBase>(GroundActor);
	if (GroundBuilding != nullptr && !GroundBuilding->CanSupportBuildingsOnTop())
	{
		Check.Result = EJ3PlacementResult::Overlapping;
		return Check;
	}


	// 해방·개수 제한·규모 상한·재료
	Check.Result = CheckCommon(BuildingId, *Def, Builder);
	if (!Check.IsOk())
	{
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
	OverlapParams.AddIgnoredActor(Anchor);
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
