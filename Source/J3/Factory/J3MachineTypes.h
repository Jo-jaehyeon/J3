// Copyright (c) ACCENT. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Inventory/J3InventoryTypes.h"
#include "J3MachineTypes.generated.h"


// 기계 칸 (machines.csv에서 상한이 0인 칸은 없다)
UENUM()
enum class EJ3MachineSlot : uint8
{
	Input,
	Fuel,
	Output,
};


// 기계가 지금 왜 돌거나 멈췄는지 (화면 안내용)
UENUM()
enum class EJ3MachineStatus : uint8
{
	Running,
	// 출력 칸이 상한까지 찼다 (컨베이어가 없으면 꺼내 가야 다시 돈다)
	OutputFull,
	// 제련할 재료가 없다
	NoInput,
	NoFuel,
	// 채굴기 아래에 캘 자원이 없다
	NoResource,
};


// 기계 상태 (서버의 공장 시뮬레이션이 들고 매 고정 틱 계산한다, Actor가 아니다)
struct FJ3MachineState
{
	int32 Id = INDEX_NONE;
	FName BuildingId;

	// 채굴기: 아래 자원 노드가 주는 아이템과 한 번에 나오는 개수
	FName ResourceItem;
	int32 ResourceYield = 1;

	FJ3ItemStack Input;
	FJ3ItemStack Fuel;
	FJ3ItemStack Output;

	// 이번 주기에 지난 시간, 지금 타고 있는 연료의 남은 시간
	float ProgressSec = 0.f;
	float FuelLeftSec = 0.f;

	// 이번 주기 길이 (제련은 레시피마다 다를 수 있다)
	float CycleSec = 0.f;

	EJ3MachineStatus Status = EJ3MachineStatus::NoInput;
};


// 클라이언트에 보이는 기계 모습 (칸 내용, 가동 상태). 바뀔 때만 복제한다.
// 진행도는 매 틱 보내지 않고, 주기 시작 시각(서버 시간)으로 클라이언트가 계산한다.
USTRUCT()
struct FJ3MachineView
{
	GENERATED_BODY()

	UPROPERTY()
	FJ3ItemStack Input;

	UPROPERTY()
	FJ3ItemStack Fuel;

	UPROPERTY()
	FJ3ItemStack Output;

	UPROPERTY()
	EJ3MachineStatus Status = EJ3MachineStatus::NoInput;

	UPROPERTY()
	float CycleSec = 0.f;

	UPROPERTY()
	double CycleStartServerSec = 0.0;

	const FJ3ItemStack& GetSlot(EJ3MachineSlot Slot) const
	{
		return Slot == EJ3MachineSlot::Input ? Input : (Slot == EJ3MachineSlot::Fuel ? Fuel : Output);
	}
};
