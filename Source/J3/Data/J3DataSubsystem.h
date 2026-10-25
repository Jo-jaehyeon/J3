#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "Containers/Ticker.h"
#include "Data/J3DataTypes.h"
#include "J3DataSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogJ3Data, Log, All);

DECLARE_MULTICAST_DELEGATE(FJ3OnDataReloaded);


// 게임 데이터(아이템·레시피·건물·설정)의 단일 창구.
// PIE 다중 클라이언트가 한 프로세스에서 같은 표를 보도록 엔진 서브시스템으로 둔다.
// 에디터에서는 Data/*.csv가 바뀌면 다시 읽고, 검증에 실패하면 이전 데이터를 그대로 유지한다.
UCLASS()
class J3_API UJ3DataSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()

public:
	static UJ3DataSubsystem* Get();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// 원본을 다시 읽는다. 오류가 있으면 반영하지 않고 false.
	UFUNCTION(BlueprintCallable, Category = "J3|Data")
	bool Reload();


	// 조회 (없으면 nullptr)
	const FJ3ItemDef* GetItem(FName ItemId) const;
	const FJ3RecipeDef* GetRecipe(FName RecipeId) const;
	const FJ3BuildingDef* GetBuilding(FName BuildingId) const;

	const TMap<FName, FJ3ItemDef>& GetAllItems() const
	{
		return Data.Items;
	}

	const TMap<FName, FJ3RecipeDef>& GetAllRecipes() const
	{
		return Data.Recipes;
	}

	const TMap<FName, FJ3BuildingDef>& GetAllBuildings() const
	{
		return Data.Buildings;
	}


	// 블루프린트(UI)용 조회
	UFUNCTION(BlueprintCallable, Category = "J3|Data")
	bool FindItem(FName ItemId, FJ3ItemDef& OutItem) const;

	UFUNCTION(BlueprintCallable, Category = "J3|Data")
	bool FindRecipe(FName RecipeId, FJ3RecipeDef& OutRecipe) const;

	UFUNCTION(BlueprintCallable, Category = "J3|Data")
	bool FindBuilding(FName BuildingId, FJ3BuildingDef& OutBuilding) const;


	// 설정 값. 키가 없으면 Fallback을 쓰고 키마다 한 번 경고한다.
	UFUNCTION(BlueprintCallable, Category = "J3|Data")
	float GetConfigFloat(FName Key, float Fallback = 0.f) const;

	UFUNCTION(BlueprintCallable, Category = "J3|Data")
	int32 GetConfigInt(FName Key, int32 Fallback = 0) const;

	// stack 칸이 비어 있으면 config의 default_stack
	UFUNCTION(BlueprintCallable, Category = "J3|Data")
	int32 GetStackSize(FName ItemId) const;

	// 서버·클라 데이터 일치 확인용
	UFUNCTION(BlueprintCallable, Category = "J3|Data")
	FString GetDataHash() const
	{
		return Data.Hash;
	}

	static FString GetDataDirectory();

	// 다시 읽기에 성공했을 때 (UI·캐시 갱신용)
	FJ3OnDataReloaded OnDataReloaded;

private:
	void ReportResult(const struct FJ3DataReport& Report, bool bApplied) const;

#if WITH_EDITOR
	void StartWatching();
	void StopWatching();
	void OnDataFilesChanged(const TArray<struct FFileChangeData>& Changes);
	bool OnReloadTick(float DeltaTime);

	FDelegateHandle WatcherHandle;
	FTSTicker::FDelegateHandle PendingReloadHandle;
#endif

	FJ3GameData Data;

	mutable TSet<FName> WarnedConfigKeys;
};
