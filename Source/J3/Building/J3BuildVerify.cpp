// Copyright (c) ACCENT. All Rights Reserved.

#include "Building/J3BuildVerify.h"

#include "Building/J3BuilderComponent.h"
#include "Building/J3BuildingBase.h"
#include "Building/J3BuildingSubsystem.h"
#include "Building/J3ConveyorBuilding.h"
#include "Building/J3MachineBuildings.h"
#include "Building/J3PowerBuildings.h"
#include "Building/J3StorageBuilding.h"
#include "Core/J3PlayerState.h"
#include "Data/J3DataSubsystem.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Factory/J3FactorySimSubsystem.h"
#include "Factory/J3MachineLogic.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "Interaction/J3ResourceNode.h"
#include "Inventory/J3InventoryComponent.h"
#include "Inventory/J3InventoryLogic.h"
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

	// 기계 사이 간격, 발전소 연료 수, 배터리로만 도는 제련 시설에 넣을 배터리·광석 수, 결과를 남길 때까지 기다리는 시간
	constexpr float FactoryGapCm = 700.f;
	constexpr int32 FactoryFuelCount = 10;
	constexpr int32 FactoryBatteryCount = 2;
	constexpr int32 FactoryBatteryOreCount = 3;
	constexpr float FactoryLogDelaySec = 40.f;
	constexpr TCHAR FactoryNodeId[] = TEXT("iron_deposit");


	// 기계의 연결점 높이 (자리 표시 모양 기준: 아이템 포트 50cm, 전기 연결점 윗면 200cm)
	constexpr float ItemPortHeightCm = 50.f;
	constexpr float PowerPortHeightCm = 200.f;
	constexpr float PortOffsetCm = 100.f;


	FVector GroundAt(UWorld* World, const FVector& Location)
	{
		FHitResult Ground;
		World->LineTraceSingleByChannel(Ground, Location + FVector(0.f, 0.f, 500.f), Location - FVector(0.f, 0.f, 2000.f), ECC_Visibility);
		return Ground.bBlockingHit ? Ground.ImpactPoint : Location;
	}


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


	FJ3ConveyorPath StraightPath(const FVector& Start, const FVector& End)
	{
		FJ3ConveyorPath Path;
		Path.Start = Start;
		Path.End = End;
		Path.StartDirection = (End - Start).GetSafeNormal();
		Path.EndDirection = Path.StartDirection;
		return Path;
	}


	void SpawnWire(UWorld* World, AJ3BuildingBase* From, AJ3BuildingBase* To)
	{
		const FJ3BuildingDef* Def = UJ3DataSubsystem::Get()->GetBuilding("power_wire");
		UClass* Class = Def != nullptr ? Def->BuildingClass.TryLoadClass<AJ3PowerWireBuilding>() : nullptr;
		if (Class == nullptr || From == nullptr || To == nullptr)
		{
			return;
		}
		const FVector Up(0.f, 0.f, PowerPortHeightCm);
		const FTransform Transform(From->GetActorLocation() + Up);
		AJ3PowerWireBuilding* Wire = World->SpawnActorDeferred<AJ3PowerWireBuilding>(Class, Transform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		Wire->SetBuildingId("power_wire");
		Wire->SetEnds(From, To, From->GetActorLocation() + Up, To->GetActorLocation() + Up);
		Wire->FinishSpawning(Transform);
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
		// 단계 5 완료 기준: 채굴 → 컨베이어 → 제련 → 창고가 전기로 돈다. 시작 지점에서 가장 가까운 철광석 광맥에 채굴기 v1을 세운다
		TActorIterator<APlayerStart> Start(World);
		const FVector Origin = Start ? Start->GetActorLocation() : FVector::ZeroVector;
		const AJ3ResourceNode* Node = nullptr;
		for (TActorIterator<AJ3ResourceNode> It(World); It; ++It)
		{
			if (It->GetNodeId() == FactoryNodeId && (Node == nullptr || FVector::Dist2D(It->GetActorLocation(), Origin) < FVector::Dist2D(Node->GetActorLocation(), Origin)))
			{
				Node = *It;
			}
		}
		if (Node == nullptr)
		{
			UE_LOG(LogJ3BuildVerify, Warning, TEXT("[공장 검증] 철광석 광맥이 없어 세우지 못했습니다"));
			return;
		}


		// 채굴기 → (컨베이어) → 제련 시설 → (컨베이어) → 창고, 발전소에서 채굴기·제련 시설로 전기줄 (창고를 마지막에 세워 다시 잇기도 확인)
		const FVector MinerAt = Node->GetActorLocation();
		const FVector SmelterAt = GroundAt(World, MinerAt + FVector(FactoryGapCm, 0.f, 0.f));
		const FVector StorageAt = GroundAt(World, SmelterAt + FVector(FactoryGapCm, 0.f, 0.f));
		const FVector GeneratorAt = GroundAt(World, SmelterAt + FVector(0.f, FactoryGapCm, 0.f));
		const FVector ItemUp(0.f, 0.f, ItemPortHeightCm);
		const FVector Front(PortOffsetCm, 0.f, 0.f);
		AJ3MinerBuilding* Miner = SpawnBuilding<AJ3MinerBuilding>(World, "miner_v1", MinerAt, nullptr);
		AJ3SmelterBuilding* Smelter = SpawnBuilding<AJ3SmelterBuilding>(World, "smelter", SmelterAt, nullptr);
		const FJ3ConveyorPath MinerToSmelter = StraightPath(MinerAt + Front + ItemUp, SmelterAt - Front + ItemUp);
		SpawnBuilding<AJ3ConveyorBuilding>(World, "conveyor", MinerToSmelter.Start, &MinerToSmelter);
		const FJ3ConveyorPath SmelterToStorage = StraightPath(SmelterAt + Front + ItemUp, StorageAt - Front + ItemUp);
		SpawnBuilding<AJ3ConveyorBuilding>(World, "conveyor", SmelterToStorage.Start, &SmelterToStorage);
		AJ3StorageBuilding* Storage = SpawnBuilding<AJ3StorageBuilding>(World, "storage", StorageAt, nullptr);
		AJ3GeneratorBuilding* Generator = SpawnBuilding<AJ3GeneratorBuilding>(World, "generator_early", GeneratorAt, nullptr);
		SpawnWire(World, Generator, Smelter);
		SpawnWire(World, Generator, Miner);


		// 발전소 연료, 배터리로만 도는 제련 시설 (전기줄 없음, Power_System 3.2 임시 전원)
		UJ3FactorySimSubsystem* Sim = World->GetSubsystem<UJ3FactorySimSubsystem>();
		FJ3MachineState* GeneratorState = Generator != nullptr ? Sim->FindMachineMutable(Generator->GetMachineId()) : nullptr;
		if (GeneratorState != nullptr)
		{
			GeneratorState->Fuel.ItemId = "log";
			GeneratorState->Fuel.Count = FactoryFuelCount;
		}
		AJ3SmelterBuilding* BatterySmelter = SpawnBuilding<AJ3SmelterBuilding>(World, "smelter", GroundAt(World, SmelterAt - FVector(0.f, FactoryGapCm, 0.f)), nullptr);
		FJ3MachineState* BatteryState = BatterySmelter != nullptr ? Sim->FindMachineMutable(BatterySmelter->GetMachineId()) : nullptr;
		if (BatteryState != nullptr)
		{
			BatteryState->Fuel.ItemId = "basic_battery";
			BatteryState->Fuel.Count = FactoryBatteryCount;
			BatteryState->Input.ItemId = "iron_ore";
			BatteryState->Input.Count = FactoryBatteryOreCount;
		}
		UE_LOG(LogJ3BuildVerify, Display, TEXT("[공장 검증] 세움: 광맥 %s, 채굴기 %s, 제련 %s, 창고 %s, 발전소 %s, 배터리 제련 %s"), *Node->GetName(), *GetNameSafe(Miner), *GetNameSafe(Smelter), *GetNameSafe(Storage), *GetNameSafe(Generator), *GetNameSafe(BatterySmelter));


		// 결과: 창고 주괴, 각 기계 상태 (상태 번호는 EJ3MachineStatus 순서: 0 가동 1 출력 가득 2 재료 없음 3 연료 없음 4 자원 없음 5 전력 없음)
		TWeakObjectPtr<AJ3StorageBuilding> WeakStorage(Storage);
		TWeakObjectPtr<AJ3MinerBuilding> WeakMiner(Miner);
		TWeakObjectPtr<AJ3SmelterBuilding> WeakSmelter(Smelter);
		TWeakObjectPtr<AJ3GeneratorBuilding> WeakGenerator(Generator);
		TWeakObjectPtr<AJ3SmelterBuilding> WeakBattery(BatterySmelter);
		FTimerHandle LogTimer;
		World->GetTimerManager().SetTimer(LogTimer, [WeakStorage, WeakMiner, WeakSmelter, WeakGenerator, WeakBattery]()
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
			auto StatusOf = [](const AJ3MachineBuilding* Machine) { return Machine != nullptr ? static_cast<int32>(Machine->GetView().Status) : -1; };
			UE_LOG(LogJ3BuildVerify, Display, TEXT("[공장 검증] 창고 철 주괴 %d | 채굴기 상태 %d | 제련 시설 상태 %d | 발전소 축전 %.0f%% 연료 %d | 배터리 제련: 주괴 %d, 남은 광석 %d, 상태 %d"),
				Ingots, StatusOf(WeakMiner.Get()), StatusOf(WeakSmelter.Get()),
				WeakGenerator.IsValid() ? WeakGenerator->GetView().EnergyFraction * 100.f : -1.f, WeakGenerator.IsValid() ? WeakGenerator->GetView().Fuel.Count : -1,
				WeakBattery.IsValid() ? WeakBattery->GetView().Output.Count : -1, WeakBattery.IsValid() ? WeakBattery->GetView().Input.Count : -1, StatusOf(WeakBattery.Get()));
		}, FactoryLogDelaySec, false);
	}, BuildDelaySec, false);
}


void J3BuildVerify::StartAutoFactoryClientLog(APlayerController* Controller)
{
	using namespace J3BuildVerifyPrivate;

	if (CVarAutoFactory.GetValueOnGameThread() <= 0 || Controller == nullptr || !Controller->IsLocalController() || Controller->GetNetMode() != NM_Client)
	{
		return;
	}
	TWeakObjectPtr<APlayerController> WeakController(Controller);
	FTimerHandle LogTimer;
	Controller->GetWorld()->GetTimerManager().SetTimer(LogTimer, [WeakController]()
	{
		// 복제로 받은 것만 본다 (창고 내용은 보는 사람에게만 가므로 든 개수만)
		UWorld* World = WeakController.IsValid() ? WeakController->GetWorld() : nullptr;
		if (World == nullptr)
		{
			return;
		}
		int32 Stored = 0;
		for (TActorIterator<AJ3StorageBuilding> It(World); It; ++It)
		{
			Stored += It->GetStoredCount();
		}
		TArray<FString> Machines;
		for (TActorIterator<AJ3MachineBuilding> It(World); It; ++It)
		{
			Machines.Add(FString::Printf(TEXT("%s=%d"), *It->GetBuildingId().ToString(), static_cast<int32>(It->GetView().Status)));
		}
		Machines.Sort();
		const UJ3FactorySimSubsystem* Sim = World->GetSubsystem<UJ3FactorySimSubsystem>();
		UE_LOG(LogJ3BuildVerify, Display, TEXT("[공장 검증] 클라이언트: 창고 %d개 | 기계 상태 %s | 벨트 구간 %d, 벨트 위 %d"), Stored, *FString::Join(Machines, TEXT(" ")), Sim != nullptr ? Sim->GetSegments().Num() : -1, Sim != nullptr ? Sim->CountItems() : -1);
	}, BuildDelaySec + FactoryLogDelaySec, false);
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
