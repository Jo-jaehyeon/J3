// Copyright (c) ACCENT. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "GameplayTagContainer.h"
#include "J3Character.generated.h"

class UAbilitySystemComponent;
class UCameraComponent;
class UJ3HarvestComponent;
class UInputAction;
class UStaticMeshComponent;
struct FInputActionValue;


// 1인칭 플레이어 캐릭터. 본인 화면에는 손만, 다른 플레이어에게는 전신 모델이 보인다.
// 메시와 입력 에셋은 블루프린트(BP_J3Character)에서 지정한다.
// GAS는 PlayerState(AJ3PlayerState)에 있고, 캐릭터는 그 아바타(Avatar)다.
UCLASS()
class J3_API AJ3Character : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AJ3Character();

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	// GAS 연결: 서버는 빙의할 때, 클라이언트는 PlayerState를 받을 때
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;

	UCameraComponent* GetFirstPersonCamera() const
	{
		return FirstPersonCamera;
	}

	UJ3HarvestComponent* GetHarvest() const
	{
		return Harvest;
	}

	// 플레이어 몸(캡슐)에서 대상 표면까지 거리. G키 거리 1m는 이 거리로 잰다.
	float GetBodyDistanceTo(const AActor* Target) const;

	// 채취 중인지 (점프·상호작용 막기, 시점 회전 범위 제한)
	bool IsHarvesting() const;

protected:
	class UJ3InventoryComponent* GetInventory() const;

	void InitAbilityActorInfo();

	void TryJump();
	void OnDeadTagChanged(const FGameplayTag Tag, int32 NewCount);

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void CycleEquipment(const FInputActionValue& Value);
	void ToggleInventory();
	void Attack();

	// G키: 조준점이 가리키는 상호작용 대상을 찾아 서버에 요청한다
	void Interact();
	AActor* FindInteractTarget() const;

	UFUNCTION(Server, Reliable)
	void Server_Interact(AActor* Target);


	// 자원 노드 채취 (G키)
	UPROPERTY(VisibleAnywhere, Category = "J3|Harvest")
	TObjectPtr<UJ3HarvestComponent> Harvest;


	// 시점
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "J3|Camera")
	TObjectPtr<UCameraComponent> FirstPersonCamera;

	// 본인 화면 전용 손 (다른 플레이어에게는 안 보임)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "J3|Mesh")
	TObjectPtr<UStaticMeshComponent> FirstPersonHands;

	// 다른 플레이어에게 보이는 전신 자리 표시 (본인에게는 안 보임).
	// TODO(아트): 스켈레탈 전신 모델이 생기면 ACharacter의 Mesh로 옮긴다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "J3|Mesh")
	TObjectPtr<UStaticMeshComponent> ThirdPersonBody;


	// 입력
	UPROPERTY(EditDefaultsOnly, Category = "J3|Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, Category = "J3|Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, Category = "J3|Input")
	TObjectPtr<UInputAction> JumpAction;

	// 마우스 휠: 장비 4칸 순환 (건설 모드에서는 회전, 단계 4)
	UPROPERTY(EditDefaultsOnly, Category = "J3|Input")
	TObjectPtr<UInputAction> CycleEquipmentAction;

	// 우클릭: 손에 든 무기의 공격 능력 (무기와 상관없이 같은 버튼)
	UPROPERTY(EditDefaultsOnly, Category = "J3|Input")
	TObjectPtr<UInputAction> AttackAction;

	// G: 상호작용 (줍기, 채취, 건물, AI 모듈)
	UPROPERTY(EditDefaultsOnly, Category = "J3|Input")
	TObjectPtr<UInputAction> InteractAction;

	// I: 인벤토리 열기
	UPROPERTY(EditDefaultsOnly, Category = "J3|Input")
	TObjectPtr<UInputAction> InventoryAction;

private:
	// OnRep_PlayerState가 여러 번 와도 태그 이벤트를 한 번만 등록하기 위해
	FDelegateHandle DeadTagHandle;
};
