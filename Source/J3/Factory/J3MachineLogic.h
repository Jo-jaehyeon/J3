// Copyright (c) ACCENT. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Factory/J3MachineTypes.h"

struct FJ3MachineDef;
struct FJ3RecipeDef;


// 기계 동작 규칙만 담은 순수 함수 모음 (자동 테스트로 검증한다).
// 채굴기: 아래 자원을 주기마다 출력 칸에 쌓는다. 제련로: 입력 칸 재료로 레시피를 돌려 출력 칸에 쌓는다 (연료 칸이 있으면 연료를 태운다).
// 컨베이어가 없으면 출력 칸이 상한까지 쌓이고, 차면 멈춘다. 플레이어가 꺼내면 다시 돈다.
namespace J3MachineLogic
{
	// 입력 아이템으로 만들 레시피 (없으면 null), 연료 한 개가 타는 시간 (연료가 아니면 0 이하)
	using FFindRecipe = TFunctionRef<const FJ3RecipeDef*(FName InputItem)>;
	using FFuelBurnSec = TFunctionRef<float(FName ItemId)>;

	// 전기를 쓰는 기계가 이번 틱에 쓸 전력을 전력망에서 받는다 (모자라면 false, 받지 않는다)
	using FTryConsumePower = TFunctionRef<bool(float Amount)>;

	// 배터리 아이템 한 개의 전력량 (배터리가 아니면 0)
	using FBatteryEnergy = TFunctionRef<float(FName ItemId)>;

	// DeltaSec만큼 진행한다. 칸 내용이나 상태가 바뀌면 true (복제할 때를 알린다).
	// 발전소는 연료를 태워 축전하고, 전기를 쓰는 기계는 돌 때만 전력을 받는다 (못 받으면 NoPower로 멈춘다)
	// 전력망에서 못 받으면 배터리 칸의 배터리로 돈다 (Power_System 3.2: 전력망이 우선, 배터리는 임시 전원)
	J3_API bool Step(FJ3MachineState& State, const FJ3MachineDef& Def, float DeltaSec, FFindRecipe FindRecipe, FFuelBurnSec FuelBurnSec, FTryConsumePower TryConsumePower, FBatteryEnergy BatteryEnergy);
	J3_API bool Step(FJ3MachineState& State, const FJ3MachineDef& Def, float DeltaSec, FFindRecipe FindRecipe, FFuelBurnSec FuelBurnSec, FTryConsumePower TryConsumePower);

	// 전력과 상관없는 경우 (전기를 쓰지 않는 기계, 테스트)
	J3_API bool Step(FJ3MachineState& State, const FJ3MachineDef& Def, float DeltaSec, FFindRecipe FindRecipe, FFuelBurnSec FuelBurnSec);

	// 아이템을 알맞은 칸(연료·배터리 → 연료 칸, 레시피 재료 → 입력 칸)에 넣는다. 넣은 개수를 돌려준다 (0이면 못 넣음)
	J3_API int32 Insert(FJ3MachineState& State, const FJ3MachineDef& Def, FName ItemId, int32 Count, FFindRecipe FindRecipe, FFuelBurnSec FuelBurnSec, FBatteryEnergy BatteryEnergy);
	J3_API int32 Insert(FJ3MachineState& State, const FJ3MachineDef& Def, FName ItemId, int32 Count, FFindRecipe FindRecipe, FFuelBurnSec FuelBurnSec);

	// 칸에서 최대 MaxCount개를 꺼낸다
	J3_API FJ3ItemStack Take(FJ3MachineState& State, EJ3MachineSlot Slot, int32 MaxCount);
}
