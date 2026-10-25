#include "Data/J3DataSubsystem.h"

#include "Data/J3CsvTable.h"
#include "Data/J3DataLoader.h"
#include "Data/J3DataValidator.h"
#include "Engine/Engine.h"
#include "Misc/Paths.h"

#if WITH_EDITOR
#include "DirectoryWatcherModule.h"
#include "IDirectoryWatcher.h"
#include "Modules/ModuleManager.h"
#endif

DEFINE_LOG_CATEGORY(LogJ3Data);


namespace J3DataSubsystemPrivate
{
	// 편집기가 저장하며 파일을 여러 번 쓰므로 마지막 변경 뒤 잠깐 기다렸다가 한 번만 읽는다
	constexpr float ReloadDebounceSec = 0.3f;

	constexpr int32 ScreenMessageKey = 0x4A3D;
}


UJ3DataSubsystem* UJ3DataSubsystem::Get()
{
	return GEngine != nullptr ? GEngine->GetEngineSubsystem<UJ3DataSubsystem>() : nullptr;
}


void UJ3DataSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Reload();

#if WITH_EDITOR
	StartWatching();
#endif
}


void UJ3DataSubsystem::Deinitialize()
{
#if WITH_EDITOR
	StopWatching();
#endif

	Super::Deinitialize();
}


FString UJ3DataSubsystem::GetDataDirectory()
{
	// TODO(기획-제안): 패키지 빌드에서 Data/를 스테이징하는 방식은 단계 8(배포)에서 정한다
	return FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(), TEXT("Data")));
}


bool UJ3DataSubsystem::Reload()
{
	// 새 데이터를 따로 읽고 검증한다. 실패하면 지금 쓰는 데이터를 건드리지 않는다.
	FJ3GameData NewData;
	FJ3DataReport Report;
	FJ3DataLoader::LoadAll(GetDataDirectory(), NewData, Report);
	if (!Report.HasErrors())
	{
		FJ3DataValidator::Validate(NewData, Report);
	}
	if (Report.HasErrors())
	{
		ReportResult(Report, false);
		return false;
	}


	// 반영
	Data = MoveTemp(NewData);
	WarnedConfigKeys.Reset();
	ReportResult(Report, true);
	OnDataReloaded.Broadcast();
	return true;
}


void UJ3DataSubsystem::ReportResult(const FJ3DataReport& Report, bool bApplied) const
{
	// 출력 로그: 오류는 줄 번호와 함께 전부, 경고는 그대로
	for (const FString& Error : Report.Errors)
	{
		UE_LOG(LogJ3Data, Error, TEXT("%s"), *Error);
	}
	for (const FString& Warning : Report.Warnings)
	{
		UE_LOG(LogJ3Data, Warning, TEXT("%s"), *Warning);
	}


	// 요약 (PIE 화면에도 띄워 CSV를 고친 사람이 바로 결과를 보게 한다)
	FString Summary;
	FColor Color = FColor::Green;
	if (bApplied)
	{
		Summary = FString::Printf(TEXT("[J3 Data] 반영됨: 아이템 %d, 레시피 %d, 건물 %d, 설정 %d (hash %s)"), Data.Items.Num(), Data.Recipes.Num(), Data.Buildings.Num(), Data.Config.Num(), *Data.Hash.Left(8));
		UE_LOG(LogJ3Data, Display, TEXT("%s"), *Summary);
	}
	else
	{
		Summary = FString::Printf(TEXT("[J3 Data] 오류 %d건, 반영 안 함 (이전 데이터 유지). 첫 오류: %s"), Report.Errors.Num(), *Report.Errors[0]);
		Color = FColor::Red;
		UE_LOG(LogJ3Data, Error, TEXT("%s"), *Summary);
	}
	if (GEngine != nullptr)
	{
		GEngine->AddOnScreenDebugMessage(J3DataSubsystemPrivate::ScreenMessageKey, 8.f, Color, Summary);
	}
}


const FJ3ItemDef* UJ3DataSubsystem::GetItem(FName ItemId) const
{
	return Data.Items.Find(ItemId);
}


const FJ3RecipeDef* UJ3DataSubsystem::GetRecipe(FName RecipeId) const
{
	return Data.Recipes.Find(RecipeId);
}


const FJ3BuildingDef* UJ3DataSubsystem::GetBuilding(FName BuildingId) const
{
	return Data.Buildings.Find(BuildingId);
}


bool UJ3DataSubsystem::FindItem(FName ItemId, FJ3ItemDef& OutItem) const
{
	const FJ3ItemDef* Found = GetItem(ItemId);
	if (Found == nullptr)
	{
		return false;
	}
	OutItem = *Found;
	return true;
}


bool UJ3DataSubsystem::FindRecipe(FName RecipeId, FJ3RecipeDef& OutRecipe) const
{
	const FJ3RecipeDef* Found = GetRecipe(RecipeId);
	if (Found == nullptr)
	{
		return false;
	}
	OutRecipe = *Found;
	return true;
}


bool UJ3DataSubsystem::FindBuilding(FName BuildingId, FJ3BuildingDef& OutBuilding) const
{
	const FJ3BuildingDef* Found = GetBuilding(BuildingId);
	if (Found == nullptr)
	{
		return false;
	}
	OutBuilding = *Found;
	return true;
}


float UJ3DataSubsystem::GetConfigFloat(FName Key, float Fallback) const
{
	const float* Value = Data.Config.Find(Key);
	if (Value != nullptr)
	{
		return *Value;
	}
	if (!WarnedConfigKeys.Contains(Key))
	{
		WarnedConfigKeys.Add(Key);
		UE_LOG(LogJ3Data, Warning, TEXT("config.csv에 '%s'이(가) 없어 기본값 %g 사용"), *Key.ToString(), Fallback);
	}
	return Fallback;
}


int32 UJ3DataSubsystem::GetConfigInt(FName Key, int32 Fallback) const
{
	return FMath::RoundToInt(GetConfigFloat(Key, static_cast<float>(Fallback)));
}


int32 UJ3DataSubsystem::GetStackSize(FName ItemId) const
{
	const FJ3ItemDef* Item = GetItem(ItemId);
	if (Item != nullptr && Item->StackSize > 0)
	{
		return Item->StackSize;
	}
	return FMath::Max(1, GetConfigInt("default_stack", 1));
}


#if WITH_EDITOR
void UJ3DataSubsystem::StartWatching()
{
	FDirectoryWatcherModule& Module = FModuleManager::LoadModuleChecked<FDirectoryWatcherModule>(TEXT("DirectoryWatcher"));
	IDirectoryWatcher* Watcher = Module.Get();
	if (Watcher == nullptr)
	{
		return;
	}
	Watcher->RegisterDirectoryChangedCallback_Handle(GetDataDirectory(), IDirectoryWatcher::FDirectoryChanged::CreateUObject(this, &UJ3DataSubsystem::OnDataFilesChanged), WatcherHandle);
}


void UJ3DataSubsystem::StopWatching()
{
	// 감시 해제
	FDirectoryWatcherModule* Module = FModuleManager::GetModulePtr<FDirectoryWatcherModule>(TEXT("DirectoryWatcher"));
	if (Module != nullptr && Module->Get() != nullptr && WatcherHandle.IsValid())
	{
		Module->Get()->UnregisterDirectoryChangedCallback_Handle(GetDataDirectory(), WatcherHandle);
	}
	WatcherHandle.Reset();


	// 대기 중인 다시 읽기 취소
	if (PendingReloadHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(PendingReloadHandle);
		PendingReloadHandle.Reset();
	}
}


void UJ3DataSubsystem::OnDataFilesChanged(const TArray<FFileChangeData>& Changes)
{
	// csv가 아닌 파일(편집기 임시 파일 등)은 무시
	const bool bCsvChanged = Changes.ContainsByPredicate([](const FFileChangeData& Change)
	{
		return FPaths::GetExtension(Change.Filename).Equals(TEXT("csv"), ESearchCase::IgnoreCase);
	});
	if (!bCsvChanged)
	{
		return;
	}


	// 마지막 변경 기준으로 다시 예약
	if (PendingReloadHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(PendingReloadHandle);
	}
	PendingReloadHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UJ3DataSubsystem::OnReloadTick), J3DataSubsystemPrivate::ReloadDebounceSec);
}


bool UJ3DataSubsystem::OnReloadTick(float DeltaTime)
{
	PendingReloadHandle.Reset();
	Reload();
	return false;
}
#endif
