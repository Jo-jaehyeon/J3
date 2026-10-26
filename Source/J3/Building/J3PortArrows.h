// Copyright (c) ACCENT. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Building/J3PortComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "J3PortArrows.generated.h"

class UMaterialInstanceDynamic;


// 설치 미리보기에서만 보이는 포트 화살표 (청사진에서 컨베이어가 어디로 들어오고 나가는지 보이고, 설치 후엔 안 보인다).
// 한 컴포넌트가 한 방향(들어옴 또는 나감)만 그린다: 들어오는 포트는 건물 쪽을, 나가는 포트는 바깥을 향한 화살표.
UCLASS(ClassGroup = (J3))
class J3_API UJ3PortArrowsComponent : public UInstancedStaticMeshComponent
{
	GENERATED_BODY()

public:
	UJ3PortArrowsComponent();

	// 이 컴포넌트가 그릴 방향과 색 (만든 쪽에서 정한다)
	void Setup(EJ3PortDirection InDirection, const FLinearColor& InColor);

	// 포트 목록 중 이 방향의 아이템 포트에 화살표를 놓는다. 위치는 이 컴포넌트 기준
	void SetPorts(const TArray<FJ3PortInfo>& Ports);

	// 화살표 크기 (기본 원뿔 100cm 기준 배율)
	UPROPERTY(EditDefaultsOnly, Category = "J3|Style")
	FVector ArrowScale = FVector(0.25f, 0.25f, 0.5f);

private:
	EJ3PortDirection Direction = EJ3PortDirection::In;
	FLinearColor Color = FLinearColor::White;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> Material;
};


namespace J3PortArrows
{
	// 들어옴·나감 화살표 색 (미리보기 공통)
	inline const FLinearColor InColor(0.2f, 1.f, 0.3f);
	inline const FLinearColor OutColor(1.f, 0.6f, 0.1f);
}
