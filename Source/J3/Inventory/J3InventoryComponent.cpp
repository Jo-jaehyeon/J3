// Copyright (c) ACCENT. All Rights Reserved.

#include "Inventory/J3InventoryComponent.h"

#include "Data/J3DataSubsystem.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "HAL/IConsoleManager.h"
#include "Inventory/J3InventoryLogic.h"
#include "Net/UnrealNetwork.h"


namespace J3InventoryComponentPrivate
{
#if !UE_BUILD_SHIPPING
	// 테스트용: 접속하면 바로 아이템을 받는다 ("아이템:수량;...")
	TAutoConsoleVariable<FString> CVarGiveOnSpawn(
		TEXT("j3.Debug.GiveOnSpawn"),
		TEXT(""),
		TEXT("접속 시 지급할 아이템 (예: stone_axe:1;bow:1;arrow:10)"));

	TAutoConsoleVariable<bool> CVarEquipOnSpawn(
		TEXT("j3.Debug.EquipOnSpawn"),
		false,
		TEXT("GiveOnSpawn으로 받은 장비를 장비 칸에 바로 넣는다"));
#endif


	int32 StackSizeOf(FName ItemId)
	{
		return UJ3DataSubsystem::Get()->GetStackSize(ItemId);
	}
}


UJ3InventoryComponent::UJ3InventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}


void UJ3InventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(UJ3InventoryComponent, Slots, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UJ3InventoryComponent, Equipment, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UJ3InventoryComponent, HeldEquipIndex, COND_OwnerOnly);
	DOREPLIFETIME(UJ3InventoryComponent, HeldItemId);
}


void UJ3InventoryComponent::BeginPlay()
{
	Super::BeginPlay();
	if (!GetOwner()->HasAuthority())
	{
		return;
	}


	// 칸 수는 설정 값 (처음 10×1, 장비 4)
	// TODO(단계 8): 저장된 프로필이 있으면 그 내용으로 복원한다
	const UJ3DataSubsystem* Data = UJ3DataSubsystem::Get();
	const int32 Columns = FMath::Max(1, Data->GetConfigInt("inventory_columns", 10));
	const int32 Rows = FMath::Max(1, Data->GetConfigInt("inventory_start_rows", 1));
	Slots.SetNum(Columns * Rows);
	Equipment.SetNum(FMath::Max(1, Data->GetConfigInt("equipment_slots", 4)));


	ApplyDebugGiveOnSpawn();
	NotifyChanged();
}


bool UJ3InventoryComponent::IsEquippable(FName ItemId)
{
	// 장비 칸에는 도구와 무기만 들어간다. 화살 같은 탄약은 인벤토리에서 소모한다 (Inventory_And_UI 2절).
	const FJ3ItemDef* Item = UJ3DataSubsystem::Get()->GetItem(ItemId);
	if (Item == nullptr)
	{
		return false;
	}
	return Item->Category == "tool" || Item->Category == "tool_weapon" || Item->Category == "weapon";
}


int32 UJ3InventoryComponent::AddItem(FName ItemId, int32 Count)
{
	check(GetOwner()->HasAuthority());

	if (UJ3DataSubsystem::Get()->GetItem(ItemId) == nullptr)
	{
		UE_LOG(LogJ3Data, Warning, TEXT("AddItem: 없는 아이템 '%s'"), *ItemId.ToString());
		return Count;
	}
	const int32 Leftover = J3InventoryLogic::Add(Slots, ItemId, Count, J3InventoryComponentPrivate::StackSizeOf);
	if (Leftover != Count)
	{
		NotifyChanged();
	}
	return Leftover;
}


bool UJ3InventoryComponent::RemoveItem(FName ItemId, int32 Count)
{
	check(GetOwner()->HasAuthority());

	if (!J3InventoryLogic::Remove(Slots, ItemId, Count))
	{
		return false;
	}
	NotifyChanged();
	return true;
}


int32 UJ3InventoryComponent::CountItem(FName ItemId) const
{
	return J3InventoryLogic::CountOf(Slots, ItemId);
}


bool UJ3InventoryComponent::EquipFromSlot(int32 SlotIndex)
{
	check(GetOwner()->HasAuthority());

	// 장비 가능한 아이템인지
	if (!Slots.IsValidIndex(SlotIndex) || Slots[SlotIndex].IsEmpty() || !IsEquippable(Slots[SlotIndex].ItemId))
	{
		return false;
	}


	// 위에서부터 첫 빈 장비 칸에 넣는다. 다 차 있으면 장착하지 않는다 (Inventory_And_UI 2절).
	const int32 EquipIndex = J3InventoryLogic::FindFirstEmpty(Equipment);
	if (EquipIndex == INDEX_NONE)
	{
		return false;
	}
	FJ3ItemStack& Source = Slots[SlotIndex];
	Equipment[EquipIndex].ItemId = Source.ItemId;
	Equipment[EquipIndex].Count = 1;
	Source.Count -= 1;
	if (Source.Count <= 0)
	{
		Source.Clear();
	}


	NotifyChanged();
	return true;
}


bool UJ3InventoryComponent::UnequipToInventory(int32 EquipIndex)
{
	check(GetOwner()->HasAuthority());

	// 인벤토리에 자리가 있을 때만 뺀다
	if (!Equipment.IsValidIndex(EquipIndex) || Equipment[EquipIndex].IsEmpty())
	{
		return false;
	}
	const FJ3ItemStack Item = Equipment[EquipIndex];
	if (J3InventoryLogic::Add(Slots, Item.ItemId, Item.Count, J3InventoryComponentPrivate::StackSizeOf) > 0)
	{
		// Add가 일부 넣었을 수 있지만 장비는 1개라 넣었거나 못 넣었거나 둘 중 하나다
		return false;
	}
	Equipment[EquipIndex].Clear();


	NotifyChanged();
	return true;
}


bool UJ3InventoryComponent::SwapEquipment(int32 EquipIndexA, int32 EquipIndexB)
{
	check(GetOwner()->HasAuthority());

	if (!Equipment.IsValidIndex(EquipIndexA) || !Equipment.IsValidIndex(EquipIndexB) || EquipIndexA == EquipIndexB)
	{
		return false;
	}
	Equipment.Swap(EquipIndexA, EquipIndexB);


	// 들고 있던 장비를 계속 들도록 손 번호도 따라간다
	if (HeldEquipIndex == EquipIndexA)
	{
		HeldEquipIndex = EquipIndexB;
	}
	else if (HeldEquipIndex == EquipIndexB)
	{
		HeldEquipIndex = EquipIndexA;
	}


	NotifyChanged();
	return true;
}


bool UJ3InventoryComponent::MoveSlot(int32 FromIndex, int32 ToIndex)
{
	check(GetOwner()->HasAuthority());

	if (!J3InventoryLogic::Move(Slots, FromIndex, ToIndex, J3InventoryComponentPrivate::StackSizeOf))
	{
		return false;
	}
	NotifyChanged();
	return true;
}


void UJ3InventoryComponent::CycleHeldEquipment(int32 Direction)
{
	check(GetOwner()->HasAuthority());

	const int32 NextIndex = J3InventoryLogic::NextActiveIndex(Equipment, HeldEquipIndex, Direction);
	if (NextIndex == HeldEquipIndex)
	{
		return;
	}
	HeldEquipIndex = NextIndex;
	NotifyChanged();
}


void UJ3InventoryComponent::Server_EquipFromSlot_Implementation(int32 SlotIndex)
{
	EquipFromSlot(SlotIndex);
}


void UJ3InventoryComponent::Server_UnequipToInventory_Implementation(int32 EquipIndex)
{
	UnequipToInventory(EquipIndex);
}


void UJ3InventoryComponent::Server_SwapEquipment_Implementation(int32 EquipIndexA, int32 EquipIndexB)
{
	SwapEquipment(EquipIndexA, EquipIndexB);
}


void UJ3InventoryComponent::Server_MoveSlot_Implementation(int32 FromIndex, int32 ToIndex)
{
	MoveSlot(FromIndex, ToIndex);
}


void UJ3InventoryComponent::Server_CycleHeldEquipment_Implementation(int32 Direction)
{
	CycleHeldEquipment(Direction);
}


void UJ3InventoryComponent::NotifyChanged()
{
	RefreshHeldItem();
	OnInventoryChanged.Broadcast();
	PrintDebugView();
}


void UJ3InventoryComponent::RefreshHeldItem()
{
	// 든 칸이 비었으면(장비를 뺐으면) 맨손으로 돌아간다
	if (HeldEquipIndex != J3BareHandIndex && (!Equipment.IsValidIndex(HeldEquipIndex) || Equipment[HeldEquipIndex].IsEmpty()))
	{
		HeldEquipIndex = J3BareHandIndex;
	}
	const FName NewHeldItemId = HeldEquipIndex == J3BareHandIndex ? NAME_None : Equipment[HeldEquipIndex].ItemId;
	if (NewHeldItemId == HeldItemId)
	{
		return;
	}
	HeldItemId = NewHeldItemId;
	OnHeldItemChanged.Broadcast(HeldItemId);
}


void UJ3InventoryComponent::OnRep_Inventory()
{
	OnInventoryChanged.Broadcast();
	PrintDebugView();
}


void UJ3InventoryComponent::OnRep_HeldItem()
{
	OnHeldItemChanged.Broadcast(HeldItemId);
}


void UJ3InventoryComponent::ApplyDebugGiveOnSpawn()
{
#if !UE_BUILD_SHIPPING
	using namespace J3InventoryComponentPrivate;

	// "아이템:수량;..." 를 지급하고, 원하면 장비까지 넣는다
	TArray<FString> Entries;
	CVarGiveOnSpawn.GetValueOnGameThread().ParseIntoArray(Entries, TEXT(";"));
	for (const FString& Entry : Entries)
	{
		FString ItemPart;
		FString CountPart;
		if (Entry.Split(TEXT(":"), &ItemPart, &CountPart))
		{
			J3InventoryLogic::Add(Slots, FName(*ItemPart.TrimStartAndEnd()), FCString::Atoi(*CountPart), StackSizeOf);
		}
	}
	if (!CVarEquipOnSpawn.GetValueOnGameThread())
	{
		return;
	}
	for (int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex)
	{
		while (!Slots[SlotIndex].IsEmpty() && IsEquippable(Slots[SlotIndex].ItemId) && EquipFromSlot(SlotIndex))
		{
		}
	}
#endif
}


void UJ3InventoryComponent::ToggleDebugView()
{
	bDebugViewVisible = !bDebugViewVisible;
	PrintDebugView();
}


void UJ3InventoryComponent::PrintDebugView() const
{
#if !UE_BUILD_SHIPPING
	// 이 PC에서 조작하는 플레이어의 인벤토리만 보여 준다
	const APlayerState* PlayerState = Cast<APlayerState>(GetOwner());
	const APlayerController* Controller = PlayerState != nullptr ? PlayerState->GetPlayerController() : nullptr;
	if (GEngine == nullptr || Controller == nullptr || !Controller->IsLocalController())
	{
		return;
	}
	const uint64 MessageKey = 0x4A30000 + GetUniqueID();
	if (!bDebugViewVisible)
	{
		GEngine->RemoveOnScreenDebugMessage(MessageKey);
		return;
	}


	// 장비 4칸(손에 든 칸 표시) + 인벤토리
	auto Describe = [](const FJ3ItemStack& Stack)
	{
		return Stack.IsEmpty() ? FString(TEXT("-")) : FString::Printf(TEXT("%s x%d"), *Stack.ItemId.ToString(), Stack.Count);
	};
	FString Text = FString::Printf(TEXT("[%s] 손: %s\n장비: "), *PlayerState->GetPlayerName(), HeldItemId.IsNone() ? TEXT("맨손") : *HeldItemId.ToString());
	for (int32 Index = 0; Index < Equipment.Num(); ++Index)
	{
		Text += FString::Printf(TEXT("%s%d:%s  "), Index == HeldEquipIndex ? TEXT(">") : TEXT(""), Index + 1, *Describe(Equipment[Index]));
	}
	Text += TEXT("\n가방: ");
	for (int32 Index = 0; Index < Slots.Num(); ++Index)
	{
		Text += FString::Printf(TEXT("%d:%s  "), Index, *Describe(Slots[Index]));
	}
	GEngine->AddOnScreenDebugMessage(MessageKey, 1000.f, FColor::Cyan, Text);
#endif
}
