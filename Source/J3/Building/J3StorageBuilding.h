// Copyright (c) ACCENT. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Building/J3BuildingBase.h"
#include "Interaction/J3Interactable.h"
#include "J3StorageBuilding.generated.h"

class AJ3PlayerController;


// 창고 (Core_Loop: 개인 인벤토리 + 공유 창고). 팀원 누구나 G키로 열어 넣고 꺼낸다.
// 내용은 서버가 들고, 화면을 연 플레이어에게만 보낸다 (Core_Loop 인벤토리와 창고). 다른 사람에게는 든 개수만 복제한다(철거 미리보기).
// 뒤(-X)에 입력 포트 (컨베이어로 넣기는 단계 5 컨베이어 작업).
UCLASS()
class J3_API AJ3StorageBuilding : public AJ3LogisticsBuilding, public IJ3Interactable
{
	GENERATED_BODY()

public:
	AJ3StorageBuilding();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual bool CanInteract(const AJ3Character* Interactor) const override;
	virtual void Interact(AJ3Character* Interactor) override;

	virtual void GetStoredItems(TArray<FJ3ItemStack>& OutItems) const override;

	virtual bool CanConnectConveyor() const override
	{
		return true;
	}

	int32 GetStoredCount() const
	{
		return StoredCount;
	}

	// 서버: 칸 내용 (넣고 꺼내기는 플레이어의 접근 컴포넌트가 이 칸을 바꾸고 NotifyChanged를 부른다)
	TArray<FJ3ItemStack>& GetSlotsMutable()
	{
		return Slots;
	}

	// 서버: 내용이 바뀌었을 때. 보고 있는 플레이어에게 다시 보낸다
	void NotifyChanged();

	// 넣을 수 있는 만큼 넣고 못 넣은 개수를 돌려준다 (컨베이어 입력도 이 함수를 쓴다)
	int32 AddItem(FName ItemId, int32 Count);

	void RemoveViewer(AJ3PlayerController* Viewer);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "J3|Building")
	TObjectPtr<UJ3PortComponent> ItemIn;

	// 서버 전용
	TArray<FJ3ItemStack> Slots;
	TArray<TWeakObjectPtr<AJ3PlayerController>> Viewers;

	// 모두에게 복제: 든 아이템 총 개수
	UPROPERTY(Replicated)
	int32 StoredCount = 0;
};
