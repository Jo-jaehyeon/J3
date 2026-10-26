// Copyright (c) ACCENT. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Inventory/J3InventoryTypes.h"
#include "J3InventoryComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FJ3OnInventoryChanged);
DECLARE_MULTICAST_DELEGATE_OneParam(FJ3OnHeldItemChanged, FName /*HeldItemId*/);


// 개인 인벤토리(10×N)와 장비 칸(4), 손에 든 장비 (Inventory_And_UI 1절).
// 상태는 서버만 바꾼다. 클라이언트는 Server_ RPC로 요청하고, 결과는 복제로 받는다.
// 플레이어 개인 데이터라 PlayerState에 붙인다 (사망·재접속에도 유지).
UCLASS(ClassGroup = (J3), meta = (BlueprintSpawnableComponent))
class J3_API UJ3InventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UJ3InventoryComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;


	// 서버 전용 변경 (게임 규칙에서 호출: 줍기, 제작, 탄약 소모 등)
	// 못 넣은 개수를 돌려준다
	int32 AddItem(FName ItemId, int32 Count);
	bool RemoveItem(FName ItemId, int32 Count);

	// 제작: 재료를 빼고 결과물을 넣는다 (모자라거나 자리가 없으면 아무것도 바꾸지 않는다)
	bool TryExchange(const TArray<struct FJ3ItemAmount>& Inputs, const struct FJ3ItemAmount& Output);

	// 건설: 재료를 모두 뺀다 (하나라도 모자라면 아무것도 빼지 않는다)
	bool TryRemoveAll(const TArray<struct FJ3ItemAmount>& Inputs);
	bool EquipFromSlot(int32 SlotIndex);
	bool UnequipToInventory(int32 EquipIndex);
	bool SwapEquipment(int32 EquipIndexA, int32 EquipIndexB);
	bool MoveSlot(int32 FromIndex, int32 ToIndex);
	void CycleHeldEquipment(int32 Direction);


	// 클라이언트 요청 (UI 클릭·드래그, 마우스 휠)
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "J3|Inventory")
	void Server_EquipFromSlot(int32 SlotIndex);

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "J3|Inventory")
	void Server_UnequipToInventory(int32 EquipIndex);

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "J3|Inventory")
	void Server_SwapEquipment(int32 EquipIndexA, int32 EquipIndexB);

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "J3|Inventory")
	void Server_MoveSlot(int32 FromIndex, int32 ToIndex);

	UFUNCTION(Server, Reliable)
	void Server_CycleHeldEquipment(int32 Direction);


	// 조회
	UFUNCTION(BlueprintCallable, Category = "J3|Inventory")
	int32 CountItem(FName ItemId) const;

	const TArray<FJ3ItemStack>& GetSlots() const
	{
		return Slots;
	}

	const TArray<FJ3ItemStack>& GetEquipment() const
	{
		return Equipment;
	}

	int32 GetHeldEquipIndex() const
	{
		return HeldEquipIndex;
	}

	FName GetHeldItemId() const
	{
		return HeldItemId;
	}

	static bool IsEquippable(FName ItemId);


	// UI 갱신용 (서버·본인 클라이언트)
	UPROPERTY(BlueprintAssignable, Category = "J3|Inventory")
	FJ3OnInventoryChanged OnInventoryChanged;

	// 손에 든 장비가 바뀜 (서버: 무기 능력 교체, 모두: 손에 든 모델 표시)
	FJ3OnHeldItemChanged OnHeldItemChanged;

protected:
	virtual void BeginPlay() override;

private:
	// 장비를 바꿀 수 있는 상태인지 (채취 중에는 불가)
	bool CanChangeEquipment() const;

	// 팀 해방 판정: 처음 얻은 아이템 기록
	void RecordObtained(FName ItemId) const;

	// 변경 뒤 공통 처리: 손에 든 장비 다시 계산, 알림
	void NotifyChanged();
	void RefreshHeldItem();
	void ApplyDebugGiveOnSpawn();

	UFUNCTION()
	void OnRep_Inventory();

	UFUNCTION()
	void OnRep_HeldItem();


	// 인벤토리·장비는 본인만 알면 된다 (VisibleInstanceOnly는 디테일 패널 디버깅용)
	UPROPERTY(VisibleInstanceOnly, ReplicatedUsing = OnRep_Inventory, Category = "J3|Inventory")
	TArray<FJ3ItemStack> Slots;

	UPROPERTY(VisibleInstanceOnly, ReplicatedUsing = OnRep_Inventory, Category = "J3|Inventory")
	TArray<FJ3ItemStack> Equipment;

	UPROPERTY(VisibleInstanceOnly, ReplicatedUsing = OnRep_Inventory, Category = "J3|Inventory")
	int32 HeldEquipIndex = J3BareHandIndex;

	// 손에 든 아이템은 다른 플레이어 화면에도 보여야 한다
	UPROPERTY(VisibleInstanceOnly, ReplicatedUsing = OnRep_HeldItem, Category = "J3|Inventory")
	FName HeldItemId;
};
