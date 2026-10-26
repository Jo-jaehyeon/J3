// Copyright (c) ACCENT. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "J3BuildPreview.generated.h"

class AJ3BuildingBase;
class UMaterialInstanceDynamic;
class UStaticMeshComponent;
class UJ3PortArrowsComponent;


// 건설 미리보기 모형 (홀로그램, Inventory_And_UI 1.3). 이 PC에서만 존재하고 복제하지 않는다.
// 설치할 수 있으면 파랑, 없으면 빨강. 충돌이 없어 판정에 끼지 않는다.
UCLASS(NotPlaceable, Transient)
class J3_API AJ3BuildPreview : public AActor
{
	GENERATED_BODY()

public:
	AJ3BuildPreview();

	// 건물 블루프린트의 모양을 그대로 따라 한다. bShowPorts: 컨베이어를 이을 수 있는 건물이면 포트 화살표를 보인다
	void SetBuildingClass(TSubclassOf<AJ3BuildingBase> BuildingClass, bool bShowPorts = false);
	void SetValid(bool bValid);

protected:
	UPROPERTY(VisibleAnywhere, Category = "J3|Building")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> Material;

	UPROPERTY(VisibleAnywhere, Category = "J3|Building")
	TObjectPtr<UJ3PortArrowsComponent> InArrows;

	UPROPERTY(VisibleAnywhere, Category = "J3|Building")
	TObjectPtr<UJ3PortArrowsComponent> OutArrows;

	UPROPERTY(EditDefaultsOnly, Category = "J3|Style")
	FLinearColor ValidColor = FLinearColor(0.1f, 0.4f, 1.f);

	UPROPERTY(EditDefaultsOnly, Category = "J3|Style")
	FLinearColor InvalidColor = FLinearColor(1.f, 0.15f, 0.1f);
};
