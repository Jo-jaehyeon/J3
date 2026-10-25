#pragma once

#include "CoreMinimal.h"
#include "J3DataTypes.generated.h"


// 아이템 ID와 수량 한 쌍. CSV의 "아이템:수량;..." 표기 한 칸에 해당한다.
USTRUCT(BlueprintType)
struct FJ3ItemAmount
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "J3|Data")
	FName ItemId;

	UPROPERTY(BlueprintReadOnly, Category = "J3|Data")
	int32 Count = 0;
};


UENUM(BlueprintType)
enum class EJ3UnlockType : uint8
{
	// 처음부터 해방
	Auto,

	// 지정 아이템 중 하나를 팀이 처음 얻을 때 해방
	OnObtain,
};


// CSV의 unlock 열 ("auto" 또는 "on_obtain:a|b")
USTRUCT(BlueprintType)
struct FJ3UnlockRule
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "J3|Data")
	EJ3UnlockType Type = EJ3UnlockType::Auto;

	// OnObtain일 때 해방 조건 아이템 (하나라도 얻으면 해방)
	UPROPERTY(BlueprintReadOnly, Category = "J3|Data")
	TArray<FName> Items;
};


// items.csv 한 행
USTRUCT(BlueprintType)
struct FJ3ItemDef
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "J3|Data")
	FName Id;

	// TODO(기획-제안): 로컬라이징 문자열 표로 분리. 지금은 name_ko를 그대로 쓴다.
	UPROPERTY(BlueprintReadOnly, Category = "J3|Data")
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "J3|Data")
	int32 Tier = 0;

	UPROPERTY(BlueprintReadOnly, Category = "J3|Data")
	FName Category;

	// 빈 칸이면 config의 default_stack을 쓴다
	UPROPERTY(BlueprintReadOnly, Category = "J3|Data")
	int32 StackSize = 0;

	UPROPERTY(BlueprintReadOnly, Category = "J3|Data")
	FString Note;
};


// recipes.csv 한 행
USTRUCT(BlueprintType)
struct FJ3RecipeDef
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "J3|Data")
	FName Id;

	UPROPERTY(BlueprintReadOnly, Category = "J3|Data")
	FJ3ItemAmount Output;

	// 제작 가능한 장소들 (ai_module, base_workbench, smelter_simple ...)
	UPROPERTY(BlueprintReadOnly, Category = "J3|Data")
	TArray<FName> Stations;

	UPROPERTY(BlueprintReadOnly, Category = "J3|Data")
	TArray<FJ3ItemAmount> Inputs;

	// 음수면 미정(빈 칸). 사용하는 쪽에서 기본값을 쓴다.
	UPROPERTY(BlueprintReadOnly, Category = "J3|Data")
	float TimeSec = -1.f;

	UPROPERTY(BlueprintReadOnly, Category = "J3|Data")
	FJ3UnlockRule Unlock;
};


// buildings.csv 한 행
USTRUCT(BlueprintType)
struct FJ3BuildingDef
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "J3|Data")
	FName Id;

	UPROPERTY(BlueprintReadOnly, Category = "J3|Data")
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "J3|Data")
	int32 Tier = 0;

	UPROPERTY(BlueprintReadOnly, Category = "J3|Data")
	TArray<FJ3ItemAmount> BuildInputs;

	UPROPERTY(BlueprintReadOnly, Category = "J3|Data")
	bool bNeedsPower = false;

	UPROPERTY(BlueprintReadOnly, Category = "J3|Data")
	FJ3UnlockRule Unlock;

	UPROPERTY(BlueprintReadOnly, Category = "J3|Data")
	FString Note;
};


// 한 번 읽어 들인 전체 게임 데이터. 검증을 통과한 것만 서브시스템에 반영된다.
struct FJ3GameData
{
	TMap<FName, FJ3ItemDef> Items;
	TMap<FName, FJ3RecipeDef> Recipes;
	TMap<FName, FJ3BuildingDef> Buildings;
	TMap<FName, float> Config;

	// 검증 오류에 원본 줄 번호를 알려 주기 위한 행 위치 (ID → CSV 줄)
	TMap<FName, int32> ItemLines;
	TMap<FName, int32> RecipeLines;
	TMap<FName, int32> BuildingLines;

	// 서버·클라 데이터 일치 확인용 (원본 파일 내용 기준)
	FString Hash;
};
