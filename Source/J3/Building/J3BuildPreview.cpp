// Copyright (c) ACCENT. All Rights Reserved.

#include "Building/J3BuildPreview.h"

#include "Building/J3BuildingBase.h"
#include "Components/StaticMeshComponent.h"
#include "J3ProjectSettings.h"
#include "Materials/MaterialInstanceDynamic.h"


AJ3BuildPreview::AJ3BuildPreview()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComponent);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCastShadow(false);
}


void AJ3BuildPreview::SetBuildingClass(TSubclassOf<AJ3BuildingBase> BuildingClass)
{
	// 건물 블루프린트 기본값의 메시·위치·배율을 복사한다
	const AJ3BuildingBase* Default = BuildingClass != nullptr ? BuildingClass->GetDefaultObject<AJ3BuildingBase>() : nullptr;
	const UStaticMeshComponent* SourceMesh = Default != nullptr ? Default->FindComponentByClass<UStaticMeshComponent>() : nullptr;
	if (SourceMesh == nullptr)
	{
		return;
	}
	Mesh->SetStaticMesh(SourceMesh->GetStaticMesh());
	Mesh->SetRelativeTransform(SourceMesh->GetRelativeTransform());


	// 모든 재질 칸을 반투명 미리보기 재질로
	UMaterialInterface* PreviewMaterial = GetDefault<UJ3ProjectSettings>()->BuildPreviewMaterial.LoadSynchronous();
	if (PreviewMaterial != nullptr)
	{
		Material = UMaterialInstanceDynamic::Create(PreviewMaterial, this);
		for (int32 Index = 0; Index < Mesh->GetNumMaterials(); ++Index)
		{
			Mesh->SetMaterial(Index, Material);
		}
	}
}


void AJ3BuildPreview::SetValid(bool bValid)
{
	if (Material != nullptr)
	{
		Material->SetVectorParameterValue("Color", bValid ? ValidColor : InvalidColor);
	}
}
