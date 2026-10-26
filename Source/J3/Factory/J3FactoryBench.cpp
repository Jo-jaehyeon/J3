// Copyright (c) ACCENT. All Rights Reserved.

#include "Factory/J3FactoryBench.h"

#include "Data/J3DataSubsystem.h"
#include "Engine/World.h"
#include "Factory/J3FactorySimSubsystem.h"
#include "HAL/IConsoleManager.h"
#include "TimerManager.h"


namespace J3FactoryBenchPrivate
{
	// 벤치마크 배치 값 (게임 수치가 아니라 측정용 고정 배치)
	constexpr float SideCm = 400.f;
	constexpr float GapCm = 200.f;
	constexpr float SpeedCmPerSec = 200.f;
	constexpr float SpacingCm = 20.f;
	constexpr float StatsIntervalSec = 2.f;
	const FVector Origin(1000.f, -2000.f, 10.f);


	TAutoConsoleVariable<FString> CVarAutoSpawn(
		TEXT("j3.Bench.AutoSpawn"),
		TEXT(""),
		TEXT("월드 시작 시 벤치마크 컨베이어를 깐다 (\"구간 수x구간당 아이템 수\", 예: 400x10)"));


	// 정사각형 고리 하나: 네 변이 차례로 이어지고 마지막 변이 첫 변으로 돌아온다
	void AddRing(UJ3FactorySimSubsystem& Simulation, const FVector& Corner, int32 FirstId, int32 ItemsPerSegment, uint16 ItemType)
	{
		const FVector Corners[4] = { Corner, Corner + FVector(SideCm, 0.f, 0.f), Corner + FVector(SideCm, SideCm, 0.f), Corner + FVector(0.f, SideCm, 0.f) };
		for (int32 Side = 0; Side < 4; ++Side)
		{
			FJ3BeltSegment Segment;
			Segment.Id = FirstId + Side;
			Segment.NextId = FirstId + (Side + 1) % 4;
			Segment.LengthCm = SideCm;
			Segment.SpeedCmPerSec = SpeedCmPerSec;
			Segment.SpacingCm = SpacingCm;
			Segment.StartLocation = Corners[Side];
			Segment.EndLocation = Corners[(Side + 1) % 4];
			for (int32 Index = 0; Index < ItemsPerSegment; ++Index)
			{
				FJ3BeltItem Item;
				Item.Type = ItemType;
				Item.Offset = SideCm - SpacingCm * (Index + 1);
				Segment.Items.Add(Item);
			}
			Simulation.AddSegment(Segment);
		}
	}


	// 몇 초마다 측정값을 로그로 남긴다
	void StartStatsLog(UJ3FactorySimSubsystem& Simulation)
	{
		TWeakObjectPtr<UJ3FactorySimSubsystem> WeakSimulation(&Simulation);
		FTimerHandle Handle;
		Simulation.GetWorld()->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateLambda([WeakSimulation]()
		{
			if (const UJ3FactorySimSubsystem* Sim = WeakSimulation.Get())
			{
				UE_LOG(LogJ3Factory, Display, TEXT("[Bench] 구간 %d, 아이템 %d, 틱 계산 %.3f ms"), Sim->GetSegments().Num(), Sim->CountItems(), Sim->GetLastStepMs());
			}
		}), StatsIntervalSec, true);
	}
}


namespace J3FactoryBench
{
	void SpawnRings(UJ3FactorySimSubsystem& Simulation, int32 SegmentCount, int32 ItemsPerSegment)
	{
		using namespace J3FactoryBenchPrivate;

		// 고리가 꽉 차면 아무것도 못 움직이므로 구간 용량보다 적게 채운다
		const int32 Capacity = FMath::FloorToInt(SideCm / SpacingCm);
		const int32 Items = FMath::Clamp(ItemsPerSegment, 0, Capacity - 1);
		const int32 RingCount = FMath::Max(1, SegmentCount / 4);
		const int32 RingsPerRow = FMath::Max(1, FMath::CeilToInt(FMath::Sqrt(static_cast<float>(RingCount))));
		const uint16 ItemType = UJ3DataSubsystem::Get()->GetItemTypeId("iron_ore");


		Simulation.ClearAll();
		for (int32 Ring = 0; Ring < RingCount; ++Ring)
		{
			const FVector Corner = Origin + FVector((Ring % RingsPerRow) * (SideCm + GapCm), (Ring / RingsPerRow) * (SideCm + GapCm), 0.f);
			AddRing(Simulation, Corner, Ring * 4, Items, ItemType);
		}
		UE_LOG(LogJ3Factory, Display, TEXT("[Bench] 고리 %d개, 구간 %d, 아이템 %d"), RingCount, Simulation.GetSegments().Num(), Simulation.CountItems());
		StartStatsLog(Simulation);
	}


	void SpawnFromConsoleVariable(UJ3FactorySimSubsystem& Simulation)
	{
#if !UE_BUILD_SHIPPING
		TArray<FString> Parts;
		J3FactoryBenchPrivate::CVarAutoSpawn.GetValueOnGameThread().ParseIntoArray(Parts, TEXT("x"));
		if (Parts.Num() == 2)
		{
			SpawnRings(Simulation, FCString::Atoi(*Parts[0]), FCString::Atoi(*Parts[1]));
		}
#endif
	}
}


#if !UE_BUILD_SHIPPING
static FAutoConsoleCommandWithWorldAndArgs GJ3BenchBeltsCommand(
	TEXT("j3.Bench.Belts"),
	TEXT("벤치마크 컨베이어를 깐다: j3.Bench.Belts <구간 수> <구간당 아이템 수> (서버에서)"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		UJ3FactorySimSubsystem* Simulation = World != nullptr ? World->GetSubsystem<UJ3FactorySimSubsystem>() : nullptr;
		if (Simulation == nullptr || Args.Num() < 2 || World->GetNetMode() == NM_Client)
		{
			return;
		}
		J3FactoryBench::SpawnRings(*Simulation, FCString::Atoi(*Args[0]), FCString::Atoi(*Args[1]));
	}));
#endif
