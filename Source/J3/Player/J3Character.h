// Copyright (c) ACCENT. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "J3Character.generated.h"

class UCameraComponent;
class UInputAction;
class UStaticMeshComponent;
struct FInputActionValue;


// 1인칭 플레이어 캐릭터. 본인 화면에는 손만, 다른 플레이어에게는 전신 모델이 보인다.
// 메시와 입력 에셋은 블루프린트(BP_J3Character)에서 지정한다.
UCLASS()
class J3_API AJ3Character : public ACharacter
{
	GENERATED_BODY()

public:
	AJ3Character();

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	UCameraComponent* GetFirstPersonCamera() const
	{
		return FirstPersonCamera;
	}

protected:
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
};
