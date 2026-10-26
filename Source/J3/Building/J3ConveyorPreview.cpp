// Copyright (c) ACCENT. All Rights Reserved.

#include "Building/J3ConveyorPreview.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Factory/J3ConveyorPath.h"
#include "J3ProjectSettings.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"


AJ3ConveyorPreview::AJ3ConveyorPreview()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Pieces = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Pieces"));
	Pieces->SetupAttachment(RootComponent);
	Pieces->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Pieces->SetCastShadow(false);
	Pieces->SetAbsolute(true, true, true);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	Pieces->SetStaticMesh(CubeMesh.Object);
}


void AJ3ConveyorPreview::SetColor(bool bValid)
{
	// 처음 쓸 때 미리보기 재질을 만든다
	if (Material == nullptr)
	{
		UMaterialInterface* PreviewMaterial = GetDefault<UJ3ProjectSettings>()->BuildPreviewMaterial.LoadSynchronous();
		Material = PreviewMaterial != nullptr ? UMaterialInstanceDynamic::Create(PreviewMaterial, this) : nullptr;
		Pieces->SetMaterial(0, Material);
	}
	if (Material != nullptr)
	{
		Material->SetVectorParameterValue("Color", bValid ? ValidColor : InvalidColor);
	}
}


void AJ3ConveyorPreview::SetMarkers(const TArray<FVector>& Locations, bool bValid)
{
	TArray<FTransform> Transforms;
	for (const FVector& Location : Locations)
	{
		Transforms.Emplace(FQuat::Identity, Location, MarkerScale);
	}
	Pieces->ClearInstances();
	Pieces->AddInstances(Transforms, false, true);
	SetColor(bValid);
}


void AJ3ConveyorPreview::SetPath(const FJ3ConveyorPath& Path, bool bValid)
{
	// 곡선을 조각으로 나눠 조각마다 상자 하나
	TArray<FVector> Points;
	TArray<float> Distances;
	Path.BuildArcTable(FMath::CeilToInt(Path.ComputeLength() / PieceLengthCm) + 1, Points, Distances);
	TArray<FTransform> Transforms;
	for (int32 Index = 1; Index < Points.Num(); ++Index)
	{
		const FVector Piece = Points[Index] - Points[Index - 1];
		Transforms.Emplace(Piece.Rotation(), (Points[Index] + Points[Index - 1]) * 0.5f, FVector(Piece.Size() / 100.f * PieceScale.X, PieceScale.Y, PieceScale.Z));
	}
	Pieces->ClearInstances();
	Pieces->AddInstances(Transforms, false, true);
	SetColor(bValid);
}
