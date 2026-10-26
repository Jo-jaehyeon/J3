// Copyright (c) ACCENT. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "GameplayTagContainer.h"
#include "J3Character.generated.h"

class UAbilitySystemComponent;
class UCameraComponent;
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

protected:
	void InitAbilityActorInfo();
	void OnDeadTagChanged(const FGameplayTag Tag, int32 NewCount);

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);


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

private:
	// OnRep_PlayerState가 여러 번 와도 태그 이벤트를 한 번만 등록하기 위해
	FDelegateHandle DeadTagHandle;
};
