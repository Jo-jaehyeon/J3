// Copyright (c) ACCENT. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "J3BuildingBase.generated.h"

class UStaticMeshComponent;
struct FJ3BuildingDef;


// 모든 건물의 기본 (Coding_Rules 3.1): 데이터 ID, 설치·철거 진입점, 공간 인덱스 등록.
// 건물 Actor는 표시·상호작용·서버 판정 진입점이고, 매 틱 계산은 공장 시뮬레이션 데이터가 맡는다.
// 모양은 블루프린트에서 지정한다. 원점이 바닥 가운데가 되도록 메시를 놓는다 (설치 판정이 원점을 바닥으로 본다).
UCLASS(Abstract)
class J3_API AJ3BuildingBase : public AActor
{
	GENERATED_BODY()

public:
	AJ3BuildingBase();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 서버: 생성 직후 종류를 정한다
	void SetBuildingId(FName InBuildingId)
	{
		BuildingId = InBuildingId;
	}

	FName GetBuildingId() const
	{
		return BuildingId;
	}

	const FJ3BuildingDef* GetDef() const;

	// 설치 판정용 크기: 원점(바닥 가운데) 기준 반 크기 (회전 전)
	FVector GetFootprintHalfExtent() const;

	// 윗면에 다른 건물을 올릴 수 있는지 (Building_Placement 1절: 충돌하지 않으면 건물 위 설치 가능)
	virtual bool CanSupportBuildingsOnTop() const
	{
		return true;
	}

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "J3|Building")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "J3|Building")
	TObjectPtr<UStaticMeshComponent> Mesh;

	// buildings.csv의 id
	UPROPERTY(Replicated, VisibleInstanceOnly, Category = "J3|Building")
	FName BuildingId;
};


// 카테고리별 기본 (건설 메뉴 탭과 같은 구분, Coding_Rules 3.1). 공통 기능은 각 단계에서 채운다.

// 생산: 채굴·제련·제조 (입출력 칸, 레시피, 가동 상태는 단계 5)
UCLASS(Abstract)
class J3_API AJ3ProductionBuilding : public AJ3BuildingBase
{
	GENERATED_BODY()
};


// 물류: 컨베이어, 창고 (단계 5)
UCLASS(Abstract)
class J3_API AJ3LogisticsBuilding : public AJ3BuildingBase
{
	GENERATED_BODY()
};


// 전력: 발전소, 전신주 (전력망 등록은 단계 5)
UCLASS(Abstract)
class J3_API AJ3PowerBuilding : public AJ3BuildingBase
{
	GENERATED_BODY()
};


// 구조물: 토대
UCLASS(Abstract)
class J3_API AJ3StructureBuilding : public AJ3BuildingBase
{
	GENERATED_BODY()
};


// 아직 기능이 없는 건물의 자리 표시 (단계 5에서 제 클래스로 바꾼다)
UCLASS()
class J3_API AJ3PlaceholderBuilding : public AJ3BuildingBase
{
	GENERATED_BODY()
};
