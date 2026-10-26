// Copyright (c) ACCENT. All Rights Reserved.

#include "Building/J3StorageBuilding.h"

#include "AbilitySystemComponent.h"
#include "Abilities/J3GameplayTags.h"
#include "Core/J3PlayerController.h"
#include "Data/J3DataSubsystem.h"
#include "Inventory/J3InventoryLogic.h"
#include "Net/UnrealNetwork.h"
#include "Player/J3Character.h"


AJ3StorageBuilding::AJ3StorageBuilding()
{
	// 기본 자리 표시 모양(2m 정육면체) 기준 뒷면 가운데. 위치는 블루프린트에서 고친다
	ItemIn = CreateDefaultSubobject<UJ3PortComponent>(TEXT("ItemIn"));
	ItemIn->SetupAttachment(Root);
	ItemIn->Direction = EJ3PortDirection::In;
	ItemIn->SetRelativeLocationAndRotation(FVector(-100.f, 0.f, 50.f), FRotator(0.f, 180.f, 0.f));
}


void AJ3StorageBuilding::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AJ3StorageBuilding, StoredCount);
}


void AJ3StorageBuilding::BeginPlay()
{
	Super::BeginPlay();

	// 칸 수는 설정 값 (TODO(기획-미결정): 창고 종류와 용량, 티어별 확장)
	if (HasAuthority())
	{
		Slots.SetNum(FMath::Max(1, UJ3DataSubsystem::Get()->GetConfigInt("storage_slots", 20)));
	}
}


bool AJ3StorageBuilding::CanInteract(const AJ3Character* Interactor) const
{
	const UAbilitySystemComponent* AbilitySystem = Interactor != nullptr ? Interactor->GetAbilitySystemComponent() : nullptr;
	return Interactor != nullptr && (AbilitySystem == nullptr || !AbilitySystem->HasMatchingGameplayTag(J3Tags::State_Dead));
}


void AJ3StorageBuilding::Interact(AJ3Character* Interactor)
{
	// 보는 사람으로 등록하고 지금 내용을 보낸다
	AJ3PlayerController* Controller = Cast<AJ3PlayerController>(Interactor->GetController());
	if (Controller == nullptr)
	{
		return;
	}
	Viewers.AddUnique(Controller);
	Controller->Client_OpenStorageScreen(this, Slots);
}


void AJ3StorageBuilding::RemoveViewer(AJ3PlayerController* Viewer)
{
	Viewers.Remove(Viewer);
}


void AJ3StorageBuilding::NotifyChanged()
{
	// 든 개수(모두)와 내용(보는 사람만)
	StoredCount = 0;
	for (const FJ3ItemStack& Stack : Slots)
	{
		StoredCount += Stack.IsEmpty() ? 0 : Stack.Count;
	}
	Viewers.RemoveAll([](const TWeakObjectPtr<AJ3PlayerController>& Viewer) { return !Viewer.IsValid(); });
	for (const TWeakObjectPtr<AJ3PlayerController>& Viewer : Viewers)
	{
		Viewer->Client_UpdateStorage(this, Slots);
	}
}


int32 AJ3StorageBuilding::AddItem(FName ItemId, int32 Count)
{
	const UJ3DataSubsystem* Data = UJ3DataSubsystem::Get();
	const int32 Leftover = J3InventoryLogic::Add(Slots, ItemId, Count, [Data](FName Item) { return Data->GetStackSize(Item); });
	if (Leftover != Count)
	{
		NotifyChanged();
	}
	return Leftover;
}


void AJ3StorageBuilding::GetStoredItems(TArray<FJ3ItemStack>& OutItems) const
{
	// 서버만 내용을 안다 (철거 환급은 서버에서 한다)
	for (const FJ3ItemStack& Stack : Slots)
	{
		if (!Stack.IsEmpty())
		{
			OutItems.Add(Stack);
		}
	}
}
