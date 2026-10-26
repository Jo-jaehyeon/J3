// Copyright (c) ACCENT. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Building/J3BuildingBase.h"
#include "Factory/J3MachineTypes.h"
#include "Interaction/J3Interactable.h"
#include "J3MachineBuildings.generated.h"

struct FJ3MachineDef;


// 기계 건물 (채굴기·제련로, Coding_Rules 3.1 AProductionBuilding). 계산은 공장 시뮬레이션이 하고(machines.csv 수치),
// 이 Actor는 등록·표시·상호작용만 맡는다. G키로 기계 화면을 열어 재료·연료를 넣고 결과를 꺼낸다.
// 컨베이어가 없으면 결과가 출력 칸 상한까지 쌓이고, 차면 멈춘다 (컨베이어 연결은 단계 5 컨베이어 작업).
UCLASS(Abstract)
class J3_API AJ3MachineBuilding : public AJ3ProductionBuilding, public IJ3Interactable
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual bool CanInteract(const AJ3Character* Interactor) const override;
	virtual void Interact(AJ3Character* Interactor) override;

	virtual void GetStoredItems(TArray<FJ3ItemStack>& OutItems) const override;
	virtual bool CanConnectConveyor() const override;

	const FJ3MachineDef* GetMachineDef() const;

	const FJ3MachineView& GetView() const
	{
		return View;
	}

	int32 GetMachineId() const
	{
		return MachineId;
	}

	// 이번 주기 진행도 0~1 (가동 중이 아니면 -1). 클라이언트는 주기 시작 시각으로 계산한다
	float GetProgress() const;

	// 서버: 시뮬레이션 상태가 바뀌었을 때 표시용 복제 값을 고친다
	void ApplyMachineState(const FJ3MachineState& State);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 채굴기: 처음 상태에 캘 자원을 넣는다
	virtual void InitMachineState(FJ3MachineState& State) const
	{
	}

	UPROPERTY(Replicated)
	FJ3MachineView View;

	// 서버: 공장 시뮬레이션 안의 기계 번호
	int32 MachineId = INDEX_NONE;
};


// 채굴기 (간이 채굴기·채굴기 v1은 같은 클래스, 차이는 데이터 행, Coding_Rules 3.1).
// 서 있는 자원 노드의 아이템을 주기마다 캔다. 앞(+X)에 출력 포트.
UCLASS()
class J3_API AJ3MinerBuilding : public AJ3MachineBuilding
{
	GENERATED_BODY()

public:
	AJ3MinerBuilding();

protected:
	virtual void InitMachineState(FJ3MachineState& State) const override;

	UPROPERTY(VisibleAnywhere, Category = "J3|Building")
	TObjectPtr<UJ3PortComponent> ItemOut;
};


// 제련로 (간이 제련로·제련 시설). recipes.csv에서 station이 이 건물 id인 레시피를 돌린다.
// 뒤(-X)에 입력, 앞(+X)에 출력 포트.
UCLASS()
class J3_API AJ3SmelterBuilding : public AJ3MachineBuilding
{
	GENERATED_BODY()

public:
	AJ3SmelterBuilding();

protected:
	UPROPERTY(VisibleAnywhere, Category = "J3|Building")
	TObjectPtr<UJ3PortComponent> ItemIn;

	UPROPERTY(VisibleAnywhere, Category = "J3|Building")
	TObjectPtr<UJ3PortComponent> ItemOut;
};
