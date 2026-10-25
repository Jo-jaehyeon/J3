// Copyright (c) ACCENT. All Rights Reserved.

#include "Player/J3Character.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"


AJ3Character::AJ3Character()
{
	// 1인칭: 몸은 컨트롤러 회전(좌우)을 따라 돌고, 카메라는 상하까지 따른다
	bUseControllerRotationYaw = true;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;


	// 시점 카메라
	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
	FirstPersonCamera->SetRelativeLocation(FVector(0.f, 0.f, BaseEyeHeight));
	FirstPersonCamera->bUsePawnControlRotation = true;


	// 본인 화면 전용 손
	FirstPersonHands = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FirstPersonHands"));
	FirstPersonHands->SetupAttachment(FirstPersonCamera);
	FirstPersonHands->SetOnlyOwnerSee(true);
	FirstPersonHands->SetCastShadow(false);
	FirstPersonHands->SetCollisionEnabled(ECollisionEnabled::NoCollision);


	// 다른 플레이어용 전신
	ThirdPersonBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ThirdPersonBody"));
	ThirdPersonBody->SetupAttachment(GetCapsuleComponent());
	ThirdPersonBody->SetOwnerNoSee(true);
	ThirdPersonBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetOwnerNoSee(true);
}


void AJ3Character::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInput = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);
	if (MoveAction != nullptr)
	{
		EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AJ3Character::Move);
	}
	if (LookAction != nullptr)
	{
		EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &AJ3Character::Look);
	}
}


void AJ3Character::Move(const FInputActionValue& Value)
{
	// X = 좌우(A/D), Y = 앞뒤(W/S). 이동 복제는 CharacterMovement가 서버 검증과 함께 처리한다.
	const FVector2D Axis = Value.Get<FVector2D>();
	AddMovementInput(GetActorForwardVector(), Axis.Y);
	AddMovementInput(GetActorRightVector(), Axis.X);
}


void AJ3Character::Look(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	AddControllerYawInput(Axis.X);
	AddControllerPitchInput(Axis.Y);
}
