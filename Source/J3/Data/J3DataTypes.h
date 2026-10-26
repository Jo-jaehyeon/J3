// Copyright (c) ACCENT. All Rights Reserved.

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


// 장비 칸에 들어가는 아이템 종류인지 (도구·무기). 탄약은 장비 칸이 아니라 인벤토리에서 소모한다 (Inventory_And_UI 2절).
inline bool J3IsEquipmentCategory(FName Category)
{
	return Category == "tool" || Category == "tool_weapon" || Category == "weapon";
}


// equipment.csv 한 행: 장비(무기)와 그 장비를 들었을 때 부여할 능력(GA), 무기 수치
USTRUCT(BlueprintType)
struct FJ3EquipmentDef
{
	GENERATED_BODY()

	// items.csv의 장비 아이템 ID
	UPROPERTY(BlueprintReadOnly, Category = "J3|Data")
	FName Id;

	// 손에 들었을 때 부여하는 GA 클래스 (블루프린트 경로)
	UPROPERTY(BlueprintReadOnly, Category = "J3|Data")
	FSoftClassPath Ability;

	UPROPERTY(BlueprintReadOnly, Category = "J3|Data")
	float Damage = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "J3|Data")
	float RangeCm = 0.f;

	// 음수면 미정(빈 칸). config의 weapon_default_cooldown_sec을 쓴다.
	UPROPERTY(BlueprintReadOnly, Category = "J3|Data")
	float CooldownSec = -1.f;

	// 쏠 때마다 1개 소모하는 탄약 (없으면 None)
	UPROPERTY(BlueprintReadOnly, Category = "J3|Data")
	FName AmmoItem;
};


// 한 번 읽어 들인 전체 게임 데이터. 검증을 통과한 것만 서브시스템에 반영된다.
struct FJ3GameData
{
	TMap<FName, FJ3ItemDef> Items;
	TMap<FName, FJ3RecipeDef> Recipes;
	TMap<FName, FJ3BuildingDef> Buildings;
	TMap<FName, FJ3EquipmentDef> Equipment;
	TMap<FName, float> Config;

	// 컨베이어 위 아이템 종류 번호 (2바이트). 아이템 ID를 이름순으로 정렬해 1부터 매긴다.
	// 서버·클라이언트는 데이터 해시가 같아야 접속되므로 번호도 같다. 0은 "없음".
	TArray<FName> ItemTypeOrder;
	TMap<FName, uint16> ItemTypeIds;

	// 검증 오류에 원본 줄 번호를 알려 주기 위한 행 위치 (ID → CSV 줄)
	TMap<FName, int32> ItemLines;
	TMap<FName, int32> RecipeLines;
	TMap<FName, int32> BuildingLines;
	TMap<FName, int32> EquipmentLines;

	// 서버·클라 데이터 일치 확인용 (원본 파일 내용 기준)
	FString Hash;
};
