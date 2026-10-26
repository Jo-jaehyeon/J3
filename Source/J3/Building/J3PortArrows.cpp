// Copyright (c) ACCENT. All Rights Reserved.

#include "Building/J3PortArrows.h"

#include "Engine/StaticMesh.h"
#include "J3ProjectSettings.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"


namespace J3PortArrowsPrivate
{
	// 화살표를 포트에서 이만큼 떨어뜨려 건물 면에 묻히지 않게 한다 (cm)
	constexpr float ArrowOffsetCm = 30.f;
}


UJ3PortArrowsComponent::UJ3PortArrowsComponent()
{
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetCastShadow(false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMesh(TEXT("/Engine/BasicShapes/Cone.Cone"));
	SetStaticMesh(ConeMesh.Object);
}


void UJ3PortArrowsComponent::Setup(EJ3PortDirection InDirection, const FLinearColor& InColor)
{
	Direction = InDirection;
	Color = InColor;
}


void UJ3PortArrowsComponent::SetPorts(const TArray<FJ3PortInfo>& Ports)
{
	using namespace J3PortArrowsPrivate;

	// 처음 쓸 때 미리보기 재질로 색을 입힌다
	if (Material == nullptr)
	{
		UMaterialInterface* PreviewMaterial = GetDefault<UJ3ProjectSettings>()->BuildPreviewMaterial.LoadSynchronous();
		Material = PreviewMaterial != nullptr ? UMaterialInstanceDynamic::Create(PreviewMaterial, this) : nullptr;
		SetMaterial(0, Material);
		if (Material != nullptr)
		{
			Material->SetVectorParameterValue("Color", Color);
		}
	}


	// 원뿔(위쪽이 뾰족)을 흐름 방향으로 눕힌다: 나감은 바깥, 들어옴은 건물 쪽
	TArray<FTransform> Transforms;
	for (const FJ3PortInfo& Port : Ports)
	{
		if (Port.Kind != EJ3PortKind::Item || Port.Direction != Direction)
		{
			continue;
		}
		const FVector Outward = Port.Transform.GetRotation().GetForwardVector();
		const FVector Flow = Direction == EJ3PortDirection::Out ? Outward : -Outward;
		Transforms.Emplace(FRotationMatrix::MakeFromZ(Flow).ToQuat(), Port.Transform.GetLocation() + Outward * ArrowOffsetCm, ArrowScale);
	}
	ClearInstances();
	AddInstances(Transforms, false, false);
}
