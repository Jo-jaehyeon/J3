// Copyright (c) ACCENT. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Factory/J3MachineTypes.h"
#include "J3MachineAccessComponent.generated.h"

class AJ3MachineBuilding;


// 플레이어가 기계에 넣고 꺼내는 요청을 서버에서 처리한다 (PlayerState에 붙는다).
// 확인: 기계 가까이 있는지(상호작용 거리 + 여유). 기계는 팀 공용이라 누구나 넣고 꺼낼 수 있다.
UCLASS(ClassGroup = (J3))
class J3_API UJ3MachineAccessComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UJ3MachineAccessComponent();

	// 가방 칸의 아이템을 알맞은 기계 칸(연료·재료)에 들어가는 만큼 넣는다
	UFUNCTION(Server, Reliable)
	void Server_InsertFromInventory(AJ3MachineBuilding* Machine, int32 InventorySlot);

	// 기계 칸의 아이템을 가방에 들어가는 만큼 꺼낸다
	UFUNCTION(Server, Reliable)
	void Server_TakeToInventory(AJ3MachineBuilding* Machine, EJ3MachineSlot Slot);

	bool IsNearMachine(const AJ3MachineBuilding* Machine) const;
};
