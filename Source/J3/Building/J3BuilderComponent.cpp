// Copyright (c) ACCENT. All Rights Reserved.

#include "Building/J3BuilderComponent.h"

#include "Building/J3BuildingBase.h"
#include "Building/J3BuildingSubsystem.h"
#include "Core/J3PlayerState.h"
#include "Data/J3DataSubsystem.h"
#include "Engine/World.h"
#include "Interaction/J3PickupItem.h"
#include "Inventory/J3InventoryComponent.h"
#include "J3ProjectSettings.h"
#include "Player/J3Character.h"

DEFINE_LOG_CATEGORY_STATIC(LogJ3Building, Log, All);


UJ3BuilderComponent::UJ3BuilderComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}


AJ3Character* UJ3BuilderComponent::GetBuilder() const
{
	const AJ3PlayerState* PlayerState = Cast<AJ3PlayerState>(GetOwner());
	return PlayerState != nullptr ? PlayerState->GetPawn<AJ3Character>() : nullptr;
}


void UJ3BuilderComponent::Server_PlaceBuilding_Implementation(FName BuildingId, FVector Location, float YawDeg, bool bGrid)
{
	// 서버 판정 (클라이언트 미리보기와 같은 함수, 결과는 서버 것만 확정)
	AJ3Character* Builder = GetBuilder();
	const UJ3BuildingSubsystem* Buildings = GetWorld()->GetSubsystem<UJ3BuildingSubsystem>();
	const FJ3PlacementCheck Check = Buildings->CheckPlacement(BuildingId, Location, YawDeg, bGrid, Builder);
	if (!Check.IsOk())
	{
		UE_LOG(LogJ3Building, Display, TEXT("설치 거부 %s: %s"), *BuildingId.ToString(), *J3Building::GetResultText(Check.Result).ToString());
		return;
	}


	// 재료 빼기 (판정과 같은 프레임이라 모자랄 수 없지만, 실패하면 짓지 않는다)
	const FJ3BuildingDef* Def = UJ3DataSubsystem::Get()->GetBuilding(BuildingId);
	UJ3InventoryComponent* Inventory = Cast<AJ3PlayerState>(GetOwner())->GetInventory();
	if (!Inventory->TryRemoveAll(Def->BuildInputs))
	{
		return;
	}


	// 생성: 종류를 정한 뒤 마쳐, 처음 복제될 때부터 종류가 들어가게 한다
	UClass* BuildingClass = Def->BuildingClass.TryLoadClass<AJ3BuildingBase>();
	AJ3BuildingBase* Building = GetWorld()->SpawnActorDeferred<AJ3BuildingBase>(BuildingClass, Check.Transform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	Building->SetBuildingId(BuildingId);
	Building->FinishSpawning(Check.Transform);
	UE_LOG(LogJ3Building, Log, TEXT("설치 %s %s"), *BuildingId.ToString(), *Check.Transform.GetLocation().ToCompactString());
}


bool UJ3BuilderComponent::CanDemolish(const AJ3BuildingBase* Building) const
{
	const AJ3Character* Builder = GetBuilder();
	return Building != nullptr && Builder != nullptr && FVector::Dist(Builder->GetActorLocation(), Building->GetActorLocation()) <= UJ3DataSubsystem::Get()->GetConfigFloat("build_range_cm", 1500.f);
}


void UJ3BuilderComponent::Server_Demolish_Implementation(AJ3BuildingBase* Building)
{
	// 거리 확인, 이미 철거 중이면 무시 (두 사람이 동시에 철거해도 한 번만 환급)
	if (!CanDemolish(Building) || Building->IsActorBeingDestroyed())
	{
		UE_LOG(LogJ3Building, Log, TEXT("철거 무시 %s"), Building != nullptr ? *Building->GetName() : TEXT("(없음)"));
		return;
	}


	// 100% 환급 + 안에 든 아이템. 가방이 차서 못 넣은 것은 건물 자리에 떨군다.
	UJ3InventoryComponent* Inventory = Cast<AJ3PlayerState>(GetOwner())->GetInventory();
	if (const FJ3BuildingDef* Def = Building->GetDef())
	{
		for (const FJ3ItemAmount& Input : Def->BuildInputs)
		{
			const int32 Leftover = Inventory->AddItem(Input.ItemId, Input.Count);
			DropLeftover(Input.ItemId, Leftover, Building->GetActorLocation());
		}
	}
	TArray<FJ3ItemStack> Stored;
	Building->GetStoredItems(Stored);
	for (const FJ3ItemStack& Stack : Stored)
	{
		const int32 Leftover = Inventory->AddItem(Stack.ItemId, Stack.Count);
		DropLeftover(Stack.ItemId, Leftover, Building->GetActorLocation());
	}
	Building->Destroy();
}


void UJ3BuilderComponent::DropLeftover(FName ItemId, int32 Count, const FVector& Location) const
{
	const TSubclassOf<AJ3PickupItem> PickupClass = GetDefault<UJ3ProjectSettings>()->PickupItemClass.LoadSynchronous();
	if (Count <= 0 || PickupClass == nullptr)
	{
		return;
	}
	FJ3ItemStack Stack;
	Stack.ItemId = ItemId;
	Stack.Count = Count;
	const FTransform Transform(Location + FVector(0.f, 0.f, 20.f));
	AJ3PickupItem* Pickup = GetWorld()->SpawnActorDeferred<AJ3PickupItem>(PickupClass, Transform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	Pickup->SetItem(Stack);
	Pickup->FinishSpawning(Transform);
}
