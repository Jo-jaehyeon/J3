// Copyright (c) ACCENT. All Rights Reserved.

#include "Interaction/J3PickupItem.h"

#include "Abilities/J3GameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/J3PlayerState.h"
#include "Inventory/J3InventoryComponent.h"
#include "Net/UnrealNetwork.h"
#include "Player/J3Character.h"


AJ3PickupItem::AJ3PickupItem()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;


	// 조준(Visibility) 판정에는 걸리고, 플레이어 이동은 막지 않는다
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	RootComponent = Mesh;
}


void AJ3PickupItem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AJ3PickupItem, Item);
}


bool AJ3PickupItem::CanInteract(const AJ3Character* Interactor) const
{
	// 이미 다른 사람이 다 주워 없어지는 중이면 안 된다
	if (Interactor == nullptr || Item.IsEmpty() || IsActorBeingDestroyed())
	{
		return false;
	}
	const UAbilitySystemComponent* AbilitySystem = Interactor->GetAbilitySystemComponent();
	return AbilitySystem == nullptr || !AbilitySystem->HasMatchingGameplayTag(J3Tags::State_Dead);
}


void AJ3PickupItem::Interact(AJ3Character* Interactor)
{
	check(HasAuthority());

	// 들어가는 만큼 인벤토리에 넣는다
	const AJ3PlayerState* PlayerState = Interactor->GetPlayerState<AJ3PlayerState>();
	if (PlayerState == nullptr)
	{
		return;
	}
	const int32 Leftover = PlayerState->GetInventory()->AddItem(Item.ItemId, Item.Count);


	// 다 들어갔으면 사라지고, 가방이 차서 남았으면 남은 개수로 바닥에 남는다
	// TODO(단계 1 UI): 가방이 가득 찼다는 안내
	if (Leftover <= 0)
	{
		Destroy();
		return;
	}
	Item.Count = Leftover;
}
