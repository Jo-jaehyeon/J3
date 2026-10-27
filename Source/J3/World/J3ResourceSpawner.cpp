// Copyright (c) ACCENT. All Rights Reserved.

#include "World/J3ResourceSpawner.h"

#include "Components/BillboardComponent.h"
#include "Core/J3GameState.h"
#include "Data/J3DataSubsystem.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "Interaction/J3ResourceNode.h"
#include "World/J3ResourcePlacement.h"

DEFINE_LOG_CATEGORY_STATIC(LogJ3World, Log, All);


AJ3ResourceSpawner::AJ3ResourceSpawner()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
#if WITH_EDITORONLY_DATA
	// 에디터 아이콘 (게임 실행(-game)에서는 만들어지지 않아 null일 수 있다)
	if (UBillboardComponent* Sprite = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Sprite")))
	{
		Sprite->SetupAttachment(RootComponent);
	}
#endif
}


FVector AJ3ResourceSpawner::GetStartLocation() const
{
	// 시작 지점 = 첫 PlayerStart (없으면 이 액터 위치)
	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		return It->GetActorLocation();
	}
	return GetActorLocation();
}


void AJ3ResourceSpawner::BeginPlay()
{
	Super::BeginPlay();
	const AJ3GameState* GameState = GetWorld()->GetGameState<AJ3GameState>();
	if (!HasAuthority() || NodeClass == nullptr || GameState == nullptr)
	{
		return;
	}


	// 배치 설정: 월드 시드에 구역 이름을 섞어 구역마다 다른 배치가 나오게 한다
	const UJ3DataSubsystem* Data = UJ3DataSubsystem::Get();
	J3ResourcePlacement::FSettings Settings;
	Settings.Seed = HashCombine(static_cast<uint32>(GameState->GetWorldSeed()), GetTypeHash(ZoneId.ToString()));
	Settings.AreaCenter = FVector2D(GetActorLocation());
	Settings.AreaHalfExtent = AreaHalfExtent;
	Settings.GuaranteeCenter = FVector2D(GetStartLocation());
	Settings.GuaranteeRadius = Data->GetConfigFloat("resource_guarantee_radius_cm", 3000.f);
	Settings.MinSpacing = Data->GetConfigFloat("resource_min_spacing_cm", 300.f);
	Settings.MaxAttempts = Data->GetConfigInt("resource_place_attempts", 32);


	// 배치 후 노드 생성
	const TArray<FJ3ResourceSpawnDef> Rules = Data->GetResourceSpawns(ZoneId);
	const TArray<J3ResourcePlacement::FPlacedNode> Placed = J3ResourcePlacement::Place(Settings, Rules);
	for (const J3ResourcePlacement::FPlacedNode& Node : Placed)
	{
		SpawnNode(Node.NodeId, Node.Location);
	}
	UE_LOG(LogJ3World, Display, TEXT("[%s] 시드 %d: 자원 노드 %d개 배치"), *ZoneId.ToString(), GameState->GetWorldSeed(), Placed.Num());
}


void AJ3ResourceSpawner::SpawnNode(FName NodeId, const FVector2D& Location)
{
	// 지면 찾기 (정적 지형만). 배치 액터 높이에서 위아래로 일정 범위만 훑는다.
	// 너무 높이서 쏘면 하늘 구체 같은 큰 메시의 바깥면에 먼저 맞는다.
	const float TraceHalfHeightCm = UJ3DataSubsystem::Get()->GetConfigFloat("resource_trace_half_height_cm", 20000.f);
	const FVector From(Location, GetActorLocation().Z + TraceHalfHeightCm);
	const FVector To(Location, GetActorLocation().Z - TraceHalfHeightCm);
	// 지형(Landscape)은 WorldStatic, 배치한 바닥 메시는 WorldDynamic일 수 있어 둘 다 찾는다. 이미 놓은 노드는 무시한다.
	FCollisionQueryParams Params(SCENE_QUERY_STAT(J3ResourceGround), true, this);
	Params.AddIgnoredActors(SpawnedNodes);
	FCollisionObjectQueryParams GroundTypes;
	GroundTypes.AddObjectTypesToQuery(ECC_WorldStatic);
	GroundTypes.AddObjectTypesToQuery(ECC_WorldDynamic);
	FHitResult Hit;
	if (!GetWorld()->LineTraceSingleByObjectType(Hit, From, To, GroundTypes, Params))
	{
		return;
	}


	// 종류를 정한 뒤 생성을 마쳐, 처음 복제될 때부터 종류가 들어가게 한다
	// 방향도 위치에서 정해 같은 시드면 모양까지 같게 한다
	const FRandomStream YawStream(GetTypeHash(Location));
	const FTransform Transform(FRotator(0.f, YawStream.FRandRange(0.f, 360.f), 0.f), Hit.ImpactPoint);
	AJ3ResourceNode* Node = GetWorld()->SpawnActorDeferred<AJ3ResourceNode>(NodeClass, Transform, this);
	Node->SetNodeId(NodeId);
	Node->FinishSpawning(Transform);
	SpawnedNodes.Add(Node);


	// 메시 바닥이 지면에 닿게 올린다 (메시 중심이 원점인 임시 모양 기준)
	FVector Origin;
	FVector Extent;
	Node->GetActorBounds(true, Origin, Extent);
	Node->AddActorWorldOffset(FVector(0.f, 0.f, Hit.ImpactPoint.Z - (Origin.Z - Extent.Z)));
}
