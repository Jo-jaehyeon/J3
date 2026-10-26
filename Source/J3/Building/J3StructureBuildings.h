// Copyright (c) ACCENT. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Building/J3BuildingBase.h"
#include "Interaction/J3Interactable.h"
#include "J3StructureBuildings.generated.h"

class UJ3CraftingStationComponent;


// 토대 (4m × 4m, Building_Placement 2.3): 경사 지형에 평평한 설치면을 만든다. 크기는 블루프린트 메시로 맞춘다.
// TODO(단계 4 정렬): 토대끼리 가장자리 스냅
UCLASS()
class J3_API AJ3Foundation : public AJ3StructureBuilding
{
	GENERATED_BODY()
};


// 기지 (Base_System): 서버당 하나, 작업대(제작) 포함. 철거·재건설 가능.
// TODO(단계 6): 부활 지점, 업그레이드, AI 모듈 이전
UCLASS()
class J3_API AJ3BaseCamp : public AJ3BuildingBase, public IJ3Interactable
{
	GENERATED_BODY()

public:
	AJ3BaseCamp();

	virtual bool CanInteract(const AJ3Character* Interactor) const override;
	virtual void Interact(AJ3Character* Interactor) override;

protected:
	// 작업대 (base_workbench)
	UPROPERTY(VisibleAnywhere, Category = "J3|Building")
	TObjectPtr<UJ3CraftingStationComponent> Workbench;
};
