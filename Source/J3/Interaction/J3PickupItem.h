// Copyright (c) ACCENT. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/J3Interactable.h"
#include "Inventory/J3InventoryTypes.h"
#include "J3PickupItem.generated.h"

class UStaticMeshComponent;


// 바닥에 떨어진 아이템. G키로 주우면 인벤토리에 들어가고, 다 들어가면 사라진다.
// 인벤토리가 가득 차서 일부만 들어가면 남은 개수로 남는다.
UCLASS()
class J3_API AJ3PickupItem : public AActor, public IJ3Interactable
{
	GENERATED_BODY()

public:
	AJ3PickupItem();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual bool CanInteract(const AJ3Character* Interactor) const override;
	virtual void Interact(AJ3Character* Interactor) override;

	const FJ3ItemStack& GetItem() const
	{
		return Item;
	}

	// 서버: 코드로 떨굴 때 (철거 환급이 가방에 다 안 들어갈 때 등)
	void SetItem(const FJ3ItemStack& InItem)
	{
		Item = InItem;
	}

protected:
	// TODO(아트): 아이템별 메시는 데이터에서 고른다. 지금은 블루프린트의 자리 표시 메시.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "J3|Pickup")
	TObjectPtr<UStaticMeshComponent> Mesh;

	// 맵에 배치할 때 지정한다 (아이템 ID와 수량)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "J3|Pickup", meta = (ExposeOnSpawn = true))
	FJ3ItemStack Item;
};
