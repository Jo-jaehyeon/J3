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

	// 건설 메뉴 탭 (production, logistics, power, structure, base, Inventory_And_UI 1.3)
	UPROPERTY(BlueprintReadOnly, Category = "J3|Data")
	FName Category;

	// 설치할 건물 클래스 (블루프린트 경로)
	UPROPERTY(BlueprintReadOnly, Category = "J3|Data")
	FSoftClassPath BuildingClass;

	// 서버에 최대 몇 개까지 (0이면 제한 없음, 기지는 1)
	UPROPERTY(BlueprintReadOnly, Category = "J3|Data")
	int32 Limit = 0;

	UPROPERTY(BlueprintReadOnly, Category = "J3|Data")
	TArray<FJ3ItemAmount> BuildInputs;

	UPROPERTY(BlueprintReadOnly, Category = "J3|Data")
	bool bNeedsPower = false;

	UPROPERTY(BlueprintReadOnly, Category = "J3|Data")
	FJ3UnlockRule Unlock;

	// 비어 있지 않으면 이 자원 노드(resource_nodes.csv id) 위에만 설치할 수 있다 (채굴기)
	UPROPERTY(BlueprintReadOnly, Category = "J3|Data")
	TArray<FName> PlaceOn;

	UPROPERTY(BlueprintReadOnly, Category = "J3|Data")
	FString Note;
};


// resource_spawns.csv 한 행: 구역(zone)별로 어떤 노드를 몇 개 놓고, 그중 몇 개를 시작 지점 반경 안에 보장할지
USTRUCT(BlueprintType)
struct FJ3ResourceSpawnDef
{
	GENERATED_BODY()

	// "구역/노드"
	UPROPERTY(BlueprintReadOnly, Category = "J3|Data")
	FName Id;

	UPROPERTY(BlueprintReadOnly, Category = "J3|Data")
	FName Zone;

	UPROPERTY(BlueprintReadOnly, Category = "J3|Data")
	FName NodeId;

	// 구역 전체에 놓을 개수 (보장 개수 포함)
	UPROPERTY(BlueprintReadOnly, Category = "J3|Data")
	int32 Count = 0;

	// 시작 지점 반경 안에 반드시 놓을 개수 (Resources_And_Tiers 1절)
	UPROPERTY(BlueprintReadOnly, Category = "J3|Data")
	int32 Guarantee = 0;
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

	// 손에 들었을 때 부여하는 GA 클래스 (블루프린트 경로). 공격이 없는 도구(곡괭이)는 비어 있다.
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

	// 채취 도구 종류 (pickaxe, axe ...). 자원 노드의 tool_tag와 같으면 채취가 빨라진다.
	UPROPERTY(BlueprintReadOnly, Category = "J3|Data")
	FName HarvestTag;

	// 채취 속도 배수 (2면 두 배 빠름)
	UPROPERTY(BlueprintReadOnly, Category = "J3|Data")
	float HarvestMultiplier = 1.f;
};


// resource_nodes.csv 한 행: 월드의 무한 자원 노드 종류 (Resources_And_Tiers 1절, 고갈 없음)
USTRUCT(BlueprintType)
struct FJ3ResourceNodeDef
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "J3|Data")
	FName Id;

	UPROPERTY(BlueprintReadOnly, Category = "J3|Data")
	FText DisplayName;

	// 채취 한 번에 얻는 아이템과 개수
	UPROPERTY(BlueprintReadOnly, Category = "J3|Data")
	FName ItemId;

	UPROPERTY(BlueprintReadOnly, Category = "J3|Data")
	int32 Yield = 1;

	// 맨손 기준 채취 한 번에 걸리는 시간
	UPROPERTY(BlueprintReadOnly, Category = "J3|Data")
	float HarvestSec = 1.f;

	// 이 종류의 도구를 들면 빨라진다 (없으면 도구 효과 없음)
	UPROPERTY(BlueprintReadOnly, Category = "J3|Data")
	FName ToolTag;

	// 이 종류의 도구가 없으면 채취할 수 없다 (없으면 맨손 가능)
	UPROPERTY(BlueprintReadOnly, Category = "J3|Data")
	FName RequiredTag;
};


// machines.csv 한 행: 생산 건물(채굴기·제련로)의 동작 수치. id는 buildings.csv의 id
USTRUCT(BlueprintType)
struct FJ3MachineDef
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "J3|Data")
	FName Id;

	// miner(노드 자원을 캔다) / smelter(레시피로 가공한다) / generator(연료로 발전·축전한다)
	UPROPERTY(BlueprintReadOnly, Category = "J3|Data")
	FName Kind;

	// 한 번 채굴·가공하는 시간 (제련은 레시피 time_sec이 있으면 그것을 쓴다)
	UPROPERTY(BlueprintReadOnly, Category = "J3|Data")
	float CycleSec = 1.f;

	// 칸별 최대 개수 (0이면 그 칸이 없다). 출력이 차면 멈춘다. 전기를 쓰는 기계의 연료 칸은 배터리 칸이다
	UPROPERTY(BlueprintReadOnly, Category = "J3|Data")
	int32 OutputCap = 0;

	UPROPERTY(BlueprintReadOnly, Category = "J3|Data")
	int32 InputCap = 0;

	UPROPERTY(BlueprintReadOnly, Category = "J3|Data")
	int32 FuelCap = 0;

	// 0이면 전기가 필요 없다 (전력은 단계 5 전력 작업에서 쓴다)
	UPROPERTY(BlueprintReadOnly, Category = "J3|Data")
	float PowerUse = 0.f;

	// 컨베이어로 넘겨줄 수 있는지 (간이 채굴기·간이 제련로는 불가)
	UPROPERTY(BlueprintReadOnly, Category = "J3|Data")
	bool bConveyor = false;

	// 발전소: 연료가 타는 동안 초당 발전량, 내부 축전 상한 (Power_System 2절)
	UPROPERTY(BlueprintReadOnly, Category = "J3|Data")
	float PowerOutput = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "J3|Data")
	float EnergyCap = 0.f;

	bool IsGenerator() const
	{
		return Kind == "generator";
	}

	// 전력망에 이어지는 건물인지 (발전소 또는 전기를 쓰는 기계)
	bool IsPowerNode() const
	{
		return IsGenerator() || PowerUse > 0.f;
	}

	bool IsMiner() const
	{
		return Kind == "miner";
	}

	bool IsSmelter() const
	{
		return Kind == "smelter";
	}
};


// fuels.csv 한 행: 연료로 쓸 수 있는 아이템과 한 개가 타는 시간. id는 items.csv의 id
USTRUCT(BlueprintType)
struct FJ3FuelDef
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "J3|Data")
	FName Id;

	UPROPERTY(BlueprintReadOnly, Category = "J3|Data")
	float BurnSec = 1.f;
};


// batteries.csv 한 행: 배터리 아이템 한 개에 든 전력량 (Power_System 3.2: 발전소가 없을 때 건물에 붙이는 임시 전원). id는 items.csv의 id
USTRUCT(BlueprintType)
struct FJ3BatteryDef
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "J3|Data")
	FName Id;

	UPROPERTY(BlueprintReadOnly, Category = "J3|Data")
	float Energy = 0.f;
};


// 한 번 읽어 들인 전체 게임 데이터. 검증을 통과한 것만 서브시스템에 반영된다.
struct FJ3GameData
{
	TMap<FName, FJ3ItemDef> Items;
	TMap<FName, FJ3RecipeDef> Recipes;
	TMap<FName, FJ3BuildingDef> Buildings;
	TMap<FName, FJ3EquipmentDef> Equipment;
	TMap<FName, FJ3ResourceNodeDef> ResourceNodes;
	TMap<FName, FJ3ResourceSpawnDef> ResourceSpawns;
	TMap<FName, FJ3MachineDef> Machines;
	TMap<FName, FJ3FuelDef> Fuels;
	TMap<FName, FJ3BatteryDef> Batteries;
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
	TMap<FName, int32> ResourceNodeLines;
	TMap<FName, int32> ResourceSpawnLines;
	TMap<FName, int32> MachineLines;
	TMap<FName, int32> FuelLines;
	TMap<FName, int32> BatteryLines;

	// 서버·클라 데이터 일치 확인용 (원본 파일 내용 기준)
	FString Hash;
};
