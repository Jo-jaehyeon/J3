// Copyright (c) ACCENT. All Rights Reserved.

#include "Building/J3BuildVerify.h"

#include "Building/J3BuilderComponent.h"
#include "Building/J3ConveyorBuilding.h"
#include "Building/J3MachineBuildings.h"
#include "Building/J3StorageBuilding.h"
#include "Data/J3DataSubsystem.h"
#include "Factory/J3FactorySimSubsystem.h"
#include "Building/J3BuildingBase.h"
#include "Building/J3BuildingSubsystem.h"
#include "Core/J3PlayerState.h"
#include "GameFramework/GameStateBase.h"
#include "Inventory/J3InventoryComponent.h"
#include "Inventory/J3InventoryLogic.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogJ3BuildVerify, Log, All);


namespace J3BuildVerifyPrivate
{
	TAutoConsoleVariable<int32> CVarAutoBuild(
		TEXT("j3.Debug.AutoBuild"),
		0,
		TEXT("0보다 크면 모든 플레이어가 시작 지점 앞 같은 자리 N곳에 동시에 토대를 짓고 짝수 번째를 부순 뒤 결과를 로그에 남긴다 (2인 검증용)"));

	// 검증 순서 (초): 접속이 끝나기를 기다렸다가 짓고, 복제가 끝나기를 기다렸다가 부수고, 다시 기다렸다가 비교한다
	constexpr float BuildDelaySec = 3.f;
	constexpr float DemolishDelaySec = 5.f;
	constexpr float LogDelaySec = 8.f;

	// 토대 한 칸(4m)씩 시작 지점 앞쪽으로
	constexpr float FirstOffsetCm = 300.f;
	constexpr float SpacingCm = 400.f;
	constexpr TCHAR BuildingId[] = TEXT("foundation");


	FVector GetSpot(UWorld* World, int32 Index)
	{
		// 시작 지점은 레벨에 놓인 액터라 서버·클라이언트 모두 같은 위치를 안다
		TActorIterator<APlayerStart> Start(World);
		const FVector Origin = Start ? Start->GetActorLocation() : FVector::ZeroVector;
		return Origin + FVector(FirstOffsetCm + Index * SpacingCm, 0.f, 0.f);
	}


	TAutoConsoleVariable<int32> CVarAutoFactory(
		TEXT("j3.Debug.AutoFactory"),
		0,
		TEXT("1이면 서버가 [컨베이어 → 제련 시설 → 컨베이어 → 창고]를 세우고 광석을 흘려 결과를 로그에 남긴다"));

	// 시작 지점 옆 (자원·줍기 물건과 겹치지 않게 옆으로 비킨 자리), 기계 사이 간격, 흘릴 광석 수
	constexpr float FactorySideOffsetCm = 1500.f;
	constexpr float FactoryGapCm = 500.f;
	constexpr int32 FactoryOreCount = 6;
	constexpr float FactoryOreIntervalSec = 0.5f;
	constexpr float FactoryLogDelaySec = 40.f;


	template <typename TBuilding>
	TBuilding* SpawnBuilding(UWorld* World, FName BuildingId, const FVector& Location, const FJ3ConveyorPath* Path)
	{
		// 설치 판정 없이 바로 세운다 (검증 전용)
		const FJ3BuildingDef* Def = UJ3DataSubsystem::Get()->GetBuilding(BuildingId);
		UClass* Class = Def != nullptr ? Def->BuildingClass.TryLoadClass<TBuilding>() : nullptr;
		if (Class == nullptr)
		{
			return nullptr;
		}
		const FTransform Transform(Location);
		TBuilding* Building = World->SpawnActorDeferred<TBuilding>(Class, Transform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		Building->SetBuildingId(BuildingId);
		if constexpr (std::is_same_v<TBuilding, AJ3ConveyorBuilding>)
		{
			Building->SetPath(*Path);
		}
		Building->FinishSpawning(Transform);
		return Building;
	}


	FAutoConsoleCommandWithWorld DumpCommand(
		TEXT("j3.Build.Dump"),
		TEXT("이 월드의 건물 개수와 해시를 로그에 남긴다"),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
		{
			J3BuildVerify::LogBuildState(World, TEXT("콘솔"));
		}));
}


int32 J3BuildVerify::GetAutoBuildCount()
{
	return J3BuildVerifyPrivate::CVarAutoBuild.GetValueOnGameThread();
}


void J3BuildVerify::StartAutoBuild(APlayerController* Controller)
{
	using namespace J3BuildVerifyPrivate;

	const int32 Count = GetAutoBuildCount();
	if (Count <= 0 || Controller == nullptr || !Controller->IsLocalController())
	{
		return;
	}
	UWorld* World = Controller->GetWorld();
	TWeakObjectPtr<APlayerController> WeakController(Controller);


	// 1) 모든 자리에 짓기 요청 (다른 플레이어도 같은 자리에 요청한다 → 서버 도착 순서로 하나만 선다)
	FTimerHandle BuildTimer;
	World->GetTimerManager().SetTimer(BuildTimer, [WeakController, Count]()
	{
		const AJ3PlayerState* PlayerState = WeakController.IsValid() ? WeakController->GetPlayerState<AJ3PlayerState>() : nullptr;
		if (PlayerState == nullptr)
		{
			return;
		}
		for (int32 Index = 0; Index < Count; ++Index)
		{
			PlayerState->GetBuilder()->Server_PlaceBuilding(BuildingId, GetSpot(WeakController->GetWorld(), Index), 0.f, false);
		}
	}, BuildDelaySec, false);


	// 2) 짝수 번째 자리의 건물을 부수기 요청 (둘이 같은 건물을 부수면 한 번만 환급되어야 한다)
	FTimerHandle DemolishTimer;
	World->GetTimerManager().SetTimer(DemolishTimer, [WeakController, Count]()
	{
		const AJ3PlayerState* PlayerState = WeakController.IsValid() ? WeakController->GetPlayerState<AJ3PlayerState>() : nullptr;
		const UJ3BuildingSubsystem* Buildings = WeakController.IsValid() ? WeakController->GetWorld()->GetSubsystem<UJ3BuildingSubsystem>() : nullptr;
		if (PlayerState == nullptr || Buildings == nullptr)
		{
			return;
		}
		for (int32 Index = 0; Index < Count; Index += 2)
		{
			const FVector Spot = GetSpot(WeakController->GetWorld(), Index);
			for (AJ3BuildingBase* Building : Buildings->FindBuildingsInBox(FBox(Spot - FVector(SpacingCm * 0.25f), Spot + FVector(SpacingCm * 0.25f))))
			{
				UE_LOG(LogJ3BuildVerify, Display, TEXT("[건설 검증] %s: 철거 요청 %s"), *PlayerState->GetPlayerName(), *Building->GetName());
				PlayerState->GetBuilder()->Server_Demolish(Building);
			}
		}
	}, DemolishDelaySec, false);


	// 3) 이 월드의 결과 기록 (서버 월드는 호스트, 클라이언트 월드는 각 클라이언트가 남긴다)
	FTimerHandle LogTimer;
	World->GetTimerManager().SetTimer(LogTimer, [WeakController]()
	{
		if (WeakController.IsValid())
		{
			LogBuildState(WeakController->GetWorld(), TEXT("자동"));
		}
	}, LogDelaySec, false);
}


void J3BuildVerify::StartAutoFactory(UWorld* World)
{
	using namespace J3BuildVerifyPrivate;

	if (CVarAutoFactory.GetValueOnGameThread() <= 0 || World == nullptr || World->GetNetMode() == NM_Client)
	{
		return;
	}
	FTimerHandle BuildTimer;
	World->GetTimerManager().SetTimer(BuildTimer, [World]()
	{
		// 바닥 높이 (시작 지점에서 아래로)
		TActorIterator<APlayerStart> Start(World);
		const FVector Origin = (Start ? Start->GetActorLocation() : FVector::ZeroVector) + FVector(0.f, FactorySideOffsetCm, 0.f);
		FHitResult Ground;
		World->LineTraceSingleByChannel(Ground, Origin + FVector(0.f, 0.f, 500.f), Origin - FVector(0.f, 0.f, 2000.f), ECC_Visibility);
		const FVector Base = Ground.bBlockingHit ? Ground.ImpactPoint : Origin;
		const float PortHeight = UJ3DataSubsystem::Get()->GetConfigFloat("conveyor_height_cm", 50.f);


		// 제련 시설 → 입력 컨베이어 → 출력 컨베이어 → 창고 순서 (창고를 나중에 세워 "나중에 지은 쪽에서 다시 잇기"도 확인)
		const FVector SmelterAt = Base;
		const FVector StorageAt = Base + FVector(FactoryGapCm + 200.f, 0.f, 0.f);
		AJ3SmelterBuilding* Smelter = SpawnBuilding<AJ3SmelterBuilding>(World, "smelter", SmelterAt, nullptr);
		FJ3ConveyorPath InPath;
		InPath.Start = SmelterAt + FVector(-100.f - FactoryGapCm, 0.f, PortHeight);
		InPath.End = SmelterAt + FVector(-100.f, 0.f, PortHeight);
		AJ3ConveyorBuilding* InConveyor = SpawnBuilding<AJ3ConveyorBuilding>(World, "conveyor", InPath.Start, &InPath);
		FJ3ConveyorPath OutPath;
		OutPath.Start = SmelterAt + FVector(100.f, 0.f, PortHeight);
		OutPath.End = StorageAt + FVector(-100.f, 0.f, PortHeight);
		SpawnBuilding<AJ3ConveyorBuilding>(World, "conveyor", OutPath.Start, &OutPath);
		AJ3StorageBuilding* Storage = SpawnBuilding<AJ3StorageBuilding>(World, "storage", StorageAt, nullptr);
		UE_LOG(LogJ3BuildVerify, Display, TEXT("[공장 검증] 세움: 제련 %s, 입력 구간 %d, 창고 %s"), *GetNameSafe(Smelter), InConveyor != nullptr ? InConveyor->GetSegmentId() : -1, *GetNameSafe(Storage));


		// 입력 컨베이어 시작에 광석을 일정 간격으로 올린다
		TWeakObjectPtr<AJ3ConveyorBuilding> WeakIn(InConveyor);
		TSharedRef<int32> Sent = MakeShared<int32>(0);
		FTimerHandle OreTimer;
		World->GetTimerManager().SetTimer(OreTimer, [World, WeakIn, Sent]()
		{
			UJ3FactorySimSubsystem* Sim = World->GetSubsystem<UJ3FactorySimSubsystem>();
			if (*Sent < FactoryOreCount && WeakIn.IsValid() && Sim->InsertItem(WeakIn->GetSegmentId(), UJ3DataSubsystem::Get()->GetItemTypeId("iron_ore")))
			{
				++(*Sent);
			}
		}, FactoryOreIntervalSec, true);


		// 결과: 창고 안 주괴 수, 제련 시설 상태
		TWeakObjectPtr<AJ3StorageBuilding> WeakStorage(Storage);
		TWeakObjectPtr<AJ3SmelterBuilding> WeakSmelter(Smelter);
		FTimerHandle LogTimer;
		World->GetTimerManager().SetTimer(LogTimer, [WeakStorage, WeakSmelter, Sent]()
		{
			TArray<FJ3ItemStack> Stored;
			if (WeakStorage.IsValid())
			{
				WeakStorage->GetStoredItems(Stored);
			}
			int32 Ingots = 0;
			for (const FJ3ItemStack& Stack : Stored)
			{
				Ingots += Stack.ItemId == "iron_ingot" ? Stack.Count : 0;
			}
			const int32 Status = WeakSmelter.IsValid() ? static_cast<int32>(WeakSmelter->GetView().Status) : -1;
			const int32 SmelterOutput = WeakSmelter.IsValid() ? WeakSmelter->GetView().Output.Count : -1;
			const UJ3FactorySimSubsystem* Sim = WeakSmelter.IsValid() ? WeakSmelter->GetWorld()->GetSubsystem<UJ3FactorySimSubsystem>() : nullptr;
			const int32 OnBelts = Sim != nullptr ? Sim->CountItems() : -1;
			UE_LOG(LogJ3BuildVerify, Display, TEXT("[공장 검증] 보낸 광석 %d, 창고 주괴 %d, 제련 시설 출력 칸 %d, 벨트 위 %d, 제련 시설 상태 %d"), *Sent, Ingots, SmelterOutput, OnBelts, Status);
		}, FactoryLogDelaySec, false);
	}, BuildDelaySec, false);
}


void J3BuildVerify::LogBuildState(UWorld* World, const TCHAR* Reason)
{
	// 종류·위치·방향을 정렬해 한 줄로 이어 해시를 만든다 (서버와 클라이언트가 같아야 한다)
	const UJ3BuildingSubsystem* Buildings = World != nullptr ? World->GetSubsystem<UJ3BuildingSubsystem>() : nullptr;
	if (Buildings == nullptr)
	{
		return;
	}
	TArray<FString> Lines;
	for (const AJ3BuildingBase* Building : Buildings->GetAllBuildings())
	{
		const FVector Location = Building->GetActorLocation();
		Lines.Add(FString::Printf(TEXT("%s@%.0f,%.0f,%.0f/%.0f"), *Building->GetBuildingId().ToString(), Location.X, Location.Y, Location.Z, Building->GetActorRotation().Yaw));
	}
	Lines.Sort();
	for (const FString& Line : Lines)
	{
		UE_LOG(LogJ3BuildVerify, Display, TEXT("[건설 검증]   %s"), *Line);
	}
	const FString Joined = FString::Join(Lines, TEXT(";"));
	const TCHAR* NetModeText = World->GetNetMode() == NM_Client ? TEXT("클라이언트") : TEXT("서버");
	UE_LOG(LogJ3BuildVerify, Display, TEXT("[건설 검증] %s %s(%s): 건물 %d개, 해시 %08X"), Reason, NetModeText, *World->GetName(), Lines.Num(), FCrc::StrCrc32(*Joined));


	// 서버: 모든 플레이어의 재료 합 (짓고 부순 만큼만 줄어야 한다. 두 번 환급되면 늘어난다)
	const AGameStateBase* GameState = World->GetGameState();
	if (World->GetNetMode() == NM_Client || GameState == nullptr)
	{
		return;
	}
	int32 TotalCount = 0;
	for (const APlayerState* Player : GameState->PlayerArray)
	{
		const AJ3PlayerState* J3Player = Cast<AJ3PlayerState>(Player);
		TotalCount += J3Player != nullptr ? J3InventoryLogic::CountOf(J3Player->GetInventory()->GetSlots(), "stone") : 0;
	}
	UE_LOG(LogJ3BuildVerify, Display, TEXT("[건설 검증] 서버: 플레이어 %d명의 돌 합계 %d"), GameState->PlayerArray.Num(), TotalCount);
}
