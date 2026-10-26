// Copyright (c) ACCENT. All Rights Reserved.

#include "Building/J3MachineBuildings.h"

#include "AbilitySystemComponent.h"
#include "Abilities/J3GameplayTags.h"
#include "Building/J3BuildingSubsystem.h"
#include "Core/J3PlayerController.h"
#include "Data/J3DataSubsystem.h"
#include "Engine/World.h"
#include "Factory/J3FactorySimSubsystem.h"
#include "GameFramework/GameStateBase.h"
#include "Interaction/J3ResourceNode.h"
#include "Net/UnrealNetwork.h"
#include "Player/J3Character.h"


namespace J3MachineBuildingsPrivate
{
	// 기본 자리 표시 모양(2m 정육면체) 기준 앞뒤 면 가운데. 위치는 블루프린트에서 고친다
	UJ3PortComponent* MakePort(AJ3BuildingBase* Owner, USceneComponent* Root, const TCHAR* Name, EJ3PortDirection Direction)
	{
		const bool bIn = Direction == EJ3PortDirection::In;
		UJ3PortComponent* Port = Owner->CreateDefaultSubobject<UJ3PortComponent>(Name);
		Port->SetupAttachment(Root);
		Port->Direction = Direction;
		Port->SetRelativeLocationAndRotation(FVector(bIn ? -100.f : 100.f, 0.f, 50.f), FRotator(0.f, bIn ? 180.f : 0.f, 0.f));
		return Port;
	}
}


void AJ3MachineBuilding::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AJ3MachineBuilding, View);
}


const FJ3MachineDef* AJ3MachineBuilding::GetMachineDef() const
{
	return UJ3DataSubsystem::Get()->GetMachine(BuildingId);
}


void AJ3MachineBuilding::BeginPlay()
{
	Super::BeginPlay();

	// 서버: 공장 시뮬레이션에 등록한다
	UJ3FactorySimSubsystem* Sim = GetWorld()->GetSubsystem<UJ3FactorySimSubsystem>();
	if (!HasAuthority() || Sim == nullptr)
	{
		return;
	}
	FJ3MachineState State;
	State.BuildingId = BuildingId;
	InitMachineState(State);
	MachineId = Sim->AddMachine(State, this);
}


void AJ3MachineBuilding::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UJ3FactorySimSubsystem* Sim = GetWorld()->GetSubsystem<UJ3FactorySimSubsystem>();
	if (Sim != nullptr && MachineId != INDEX_NONE)
	{
		Sim->RemoveMachine(MachineId);
	}
	Super::EndPlay(EndPlayReason);
}


void AJ3MachineBuilding::ApplyMachineState(const FJ3MachineState& State)
{
	// 주기 시작 시각 = 지금 - 진행한 시간 (클라이언트가 진행 막대를 스스로 그린다)
	const AGameStateBase* GameState = GetWorld()->GetGameState();
	View.Input = State.Input;
	View.Fuel = State.Fuel;
	View.Output = State.Output;
	View.Status = State.Status;
	View.CycleSec = State.CycleSec;
	View.CycleStartServerSec = (GameState != nullptr ? GameState->GetServerWorldTimeSeconds() : 0.0) - State.ProgressSec;
}


float AJ3MachineBuilding::GetProgress() const
{
	const AGameStateBase* GameState = GetWorld()->GetGameState();
	if (View.Status != EJ3MachineStatus::Running || View.CycleSec <= 0.f || GameState == nullptr)
	{
		return -1.f;
	}
	return FMath::Clamp(static_cast<float>((GameState->GetServerWorldTimeSeconds() - View.CycleStartServerSec) / View.CycleSec), 0.f, 1.f);
}


void AJ3MachineBuilding::GetStoredItems(TArray<FJ3ItemStack>& OutItems) const
{
	// 철거 환급·미리보기용 (복제된 값이라 서버·클라이언트가 같다)
	for (const FJ3ItemStack& Stack : { View.Input, View.Fuel, View.Output })
	{
		if (!Stack.IsEmpty())
		{
			OutItems.Add(Stack);
		}
	}
}


bool AJ3MachineBuilding::CanInteract(const AJ3Character* Interactor) const
{
	const UAbilitySystemComponent* AbilitySystem = Interactor != nullptr ? Interactor->GetAbilitySystemComponent() : nullptr;
	return Interactor != nullptr && GetMachineDef() != nullptr && (AbilitySystem == nullptr || !AbilitySystem->HasMatchingGameplayTag(J3Tags::State_Dead));
}


void AJ3MachineBuilding::Interact(AJ3Character* Interactor)
{
	if (AJ3PlayerController* Controller = Cast<AJ3PlayerController>(Interactor->GetController()))
	{
		Controller->Client_OpenMachineScreen(this);
	}
}


AJ3MinerBuilding::AJ3MinerBuilding()
{
	ItemOut = J3MachineBuildingsPrivate::MakePort(this, Root, TEXT("ItemOut"), EJ3PortDirection::Out);
}


void AJ3MinerBuilding::InitMachineState(FJ3MachineState& State) const
{
	// 서 있는 자원 노드 (설치 판정과 같은 함수, 자기 자신은 빼고 본다)
	const FJ3BuildingDef* Def = GetDef();
	const UJ3BuildingSubsystem* Buildings = GetWorld()->GetSubsystem<UJ3BuildingSubsystem>();
	const AJ3ResourceNode* Node = Def != nullptr && Buildings != nullptr ? Cast<AJ3ResourceNode>(Buildings->FindAnchorNode(Def->PlaceOn, GetActorLocation(), this)) : nullptr;
	const FJ3ResourceNodeDef* NodeDef = Node != nullptr ? Node->GetDef() : nullptr;
	if (NodeDef != nullptr)
	{
		State.ResourceItem = NodeDef->ItemId;
		State.ResourceYield = NodeDef->Yield;
	}
}


AJ3SmelterBuilding::AJ3SmelterBuilding()
{
	ItemIn = J3MachineBuildingsPrivate::MakePort(this, Root, TEXT("ItemIn"), EJ3PortDirection::In);
	ItemOut = J3MachineBuildingsPrivate::MakePort(this, Root, TEXT("ItemOut"), EJ3PortDirection::Out);
}
