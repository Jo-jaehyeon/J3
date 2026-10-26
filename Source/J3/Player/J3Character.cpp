// Copyright (c) ACCENT. All Rights Reserved.

#include "Player/J3Character.h"

#include "AbilitySystemComponent.h"
#include "Abilities/J3GameplayTags.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/J3PlayerState.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputActionValue.h"
#include "Inventory/J3InventoryComponent.h"


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
	if (JumpAction != nullptr)
	{
		// 점프 높이·체공은 CharacterMovement 설정(BP_J3Character)에서 조정한다
		EnhancedInput->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInput->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
	}
	if (CycleEquipmentAction != nullptr)
	{
		EnhancedInput->BindAction(CycleEquipmentAction, ETriggerEvent::Triggered, this, &AJ3Character::CycleEquipment);
	}
	if (InventoryAction != nullptr)
	{
		EnhancedInput->BindAction(InventoryAction, ETriggerEvent::Started, this, &AJ3Character::ToggleInventory);
	}
}


UJ3InventoryComponent* AJ3Character::GetInventory() const
{
	const AJ3PlayerState* J3PlayerState = GetPlayerState<AJ3PlayerState>();
	return J3PlayerState != nullptr ? J3PlayerState->GetInventory() : nullptr;
}


UAbilitySystemComponent* AJ3Character::GetAbilitySystemComponent() const
{
	const AJ3PlayerState* J3PlayerState = GetPlayerState<AJ3PlayerState>();
	return J3PlayerState != nullptr ? J3PlayerState->GetAbilitySystemComponent() : nullptr;
}


void AJ3Character::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	InitAbilityActorInfo();
}


void AJ3Character::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	InitAbilityActorInfo();
}


void AJ3Character::InitAbilityActorInfo()
{
	// 주인(Owner)은 PlayerState, 몸(Avatar)은 이 캐릭터
	AJ3PlayerState* J3PlayerState = GetPlayerState<AJ3PlayerState>();
	UAbilitySystemComponent* AbilitySystem = GetAbilitySystemComponent();
	if (AbilitySystem == nullptr)
	{
		return;
	}
	AbilitySystem->InitAbilityActorInfo(J3PlayerState, this);


	// 사망 태그에 맞춰 이동을 막는다. 태그는 복제되므로 서버·본인·다른 클라이언트 모두 같은 처리를 한다.
	if (!DeadTagHandle.IsValid())
	{
		DeadTagHandle = AbilitySystem->RegisterGameplayTagEvent(J3Tags::State_Dead, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &AJ3Character::OnDeadTagChanged);
	}
	OnDeadTagChanged(J3Tags::State_Dead, AbilitySystem->GetTagCount(J3Tags::State_Dead));
}


void AJ3Character::OnDeadTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	// TODO(단계 6): 쓰러짐 연출, 부활 버튼·제세동기
	if (NewCount > 0)
	{
		GetCharacterMovement()->DisableMovement();
	}
	else
	{
		GetCharacterMovement()->SetDefaultMovementMode();
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


void AJ3Character::CycleEquipment(const FInputActionValue& Value)
{
	// 휠 한 칸 = 한 칸 이동. 바뀐 결과(손에 든 장비)는 서버가 정하고 복제한다.
	const float Axis = Value.Get<float>();
	UJ3InventoryComponent* Inventory = GetInventory();
	if (Inventory == nullptr || FMath::IsNearlyZero(Axis))
	{
		return;
	}
	Inventory->Server_CycleHeldEquipment(Axis > 0.f ? -1 : 1);
}


void AJ3Character::ToggleInventory()
{
	// TODO(단계 1 UI): 인벤토리 화면(UMG)을 연다. 지금은 화면에 내용만 띄운다.
	if (UJ3InventoryComponent* Inventory = GetInventory())
	{
		Inventory->ToggleDebugView();
	}
}
