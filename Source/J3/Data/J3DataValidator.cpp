// Copyright (c) ACCENT. All Rights Reserved.

#include "Data/J3DataValidator.h"

#include "Algo/AllOf.h"
#include "Data/J3CsvTable.h"
#include "Misc/PackageName.h"


namespace J3DataValidatorPrivate
{
	int32 LineOf(const TMap<FName, int32>& Lines, FName Id)
	{
		const int32* Line = Lines.Find(Id);
		return Line != nullptr ? *Line : 0;
	}


	void CheckItem(const FJ3GameData& Data, FJ3DataReport& Report, const TCHAR* File, int32 Line, const TCHAR* Column, FName ItemId)
	{
		if (!Data.Items.Contains(ItemId))
		{
			Report.AddError(File, Line, FString::Printf(TEXT("%s: 없는 아이템 '%s'"), Column, *ItemId.ToString()));
		}
	}


	bool IsUnlockReachable(const FJ3UnlockRule& Rule, const TSet<FName>& Obtainable)
	{
		if (Rule.Type == EJ3UnlockType::Auto)
		{
			return true;
		}
		for (const FName& ItemId : Rule.Items)
		{
			if (Obtainable.Contains(ItemId))
			{
				return true;
			}
		}
		return false;
	}
}


void FJ3DataValidator::Validate(const FJ3GameData& Data, FJ3DataReport& Report)
{
	CheckReferences(Data, Report);
	CheckEquipment(Data, Report);
	CheckResourceNodes(Data, Report);


	// 참조가 깨져 있으면 도달 검사 결과가 오류를 중복으로 쏟아내므로 건너뛴다
	if (!Report.HasErrors())
	{
		CheckReachability(Data, Report);
	}


	WarnUndecided(Data, Report);
}


void FJ3DataValidator::CheckReferences(const FJ3GameData& Data, FJ3DataReport& Report)
{
	using namespace J3DataValidatorPrivate;

	// 레시피의 출력·재료·해방 조건
	for (const TPair<FName, FJ3RecipeDef>& Pair : Data.Recipes)
	{
		const FJ3RecipeDef& Recipe = Pair.Value;
		const int32 Line = LineOf(Data.RecipeLines, Recipe.Id);
		CheckItem(Data, Report, TEXT("recipes.csv"), Line, TEXT("output"), Recipe.Output.ItemId);
		for (const FJ3ItemAmount& Input : Recipe.Inputs)
		{
			CheckItem(Data, Report, TEXT("recipes.csv"), Line, TEXT("inputs"), Input.ItemId);
		}
		for (const FName& ItemId : Recipe.Unlock.Items)
		{
			CheckItem(Data, Report, TEXT("recipes.csv"), Line, TEXT("unlock"), ItemId);
		}
	}


	// 건물의 건설 재료·해방 조건
	for (const TPair<FName, FJ3BuildingDef>& Pair : Data.Buildings)
	{
		const FJ3BuildingDef& Building = Pair.Value;
		const int32 Line = LineOf(Data.BuildingLines, Building.Id);
		for (const FJ3ItemAmount& Input : Building.BuildInputs)
		{
			CheckItem(Data, Report, TEXT("buildings.csv"), Line, TEXT("build_inputs"), Input.ItemId);
		}
		for (const FName& ItemId : Building.Unlock.Items)
		{
			CheckItem(Data, Report, TEXT("buildings.csv"), Line, TEXT("unlock"), ItemId);
		}
		for (const FName& NodeId : Building.PlaceOn)
		{
			if (!Data.ResourceNodes.Contains(NodeId))
			{
				Report.AddError(TEXT("buildings.csv"), Line, FString::Printf(TEXT("place_on: 없는 자원 노드 '%s'"), *NodeId.ToString()));
			}
		}
		if (!FPackageName::DoesPackageExist(Building.BuildingClass.GetLongPackageName()))
		{
			Report.AddError(TEXT("buildings.csv"), Line, FString::Printf(TEXT("class: 없는 건물 블루프린트 '%s'"), *Building.BuildingClass.ToString()));
		}
	}
}


void FJ3DataValidator::CheckEquipment(const FJ3GameData& Data, FJ3DataReport& Report)
{
	using namespace J3DataValidatorPrivate;

	for (const TPair<FName, FJ3EquipmentDef>& Pair : Data.Equipment)
	{
		// 장비 아이템이 있고 장비 칸에 들어가는 종류인지
		const FJ3EquipmentDef& Equipment = Pair.Value;
		const int32 Line = LineOf(Data.EquipmentLines, Equipment.Id);
		const FJ3ItemDef* Item = Data.Items.Find(Equipment.Id);
		if (Item == nullptr)
		{
			Report.AddError(TEXT("equipment.csv"), Line, FString::Printf(TEXT("id: 없는 아이템 '%s'"), *Equipment.Id.ToString()));
		}
		else if (!J3IsEquipmentCategory(Item->Category))
		{
			Report.AddError(TEXT("equipment.csv"), Line, FString::Printf(TEXT("id: '%s'은(는) 장비가 아닙니다 (category %s)"), *Equipment.Id.ToString(), *Item->Category.ToString()));
		}


		// GA 블루프린트가 실제로 있는지 (비어 있으면 공격 없는 도구, 클래스 종류는 장착할 때 확인한다)
		const FString PackageName = Equipment.Ability.GetLongPackageName();
		if (!Equipment.Ability.IsNull() && !FPackageName::DoesPackageExist(PackageName))
		{
			Report.AddError(TEXT("equipment.csv"), Line, FString::Printf(TEXT("ability: 없는 GA 경로 '%s'"), *Equipment.Ability.ToString()));
		}


		// 탄약
		if (!Equipment.AmmoItem.IsNone())
		{
			CheckItem(Data, Report, TEXT("equipment.csv"), Line, TEXT("ammo"), Equipment.AmmoItem);
		}
	}
}


void FJ3DataValidator::CheckResourceNodes(const FJ3GameData& Data, FJ3DataReport& Report)
{
	using namespace J3DataValidatorPrivate;

	// 노드가 주는 아이템이 있어야 한다
	for (const TPair<FName, FJ3ResourceNodeDef>& Pair : Data.ResourceNodes)
	{
		CheckItem(Data, Report, TEXT("resource_nodes.csv"), LineOf(Data.ResourceNodeLines, Pair.Key), TEXT("item"), Pair.Value.ItemId);
	}


	// 배치 규칙의 노드가 있어야 한다
	for (const TPair<FName, FJ3ResourceSpawnDef>& Pair : Data.ResourceSpawns)
	{
		if (!Data.ResourceNodes.Contains(Pair.Value.NodeId))
		{
			Report.AddError(TEXT("resource_spawns.csv"), LineOf(Data.ResourceSpawnLines, Pair.Key), FString::Printf(TEXT("node: 없는 자원 노드 '%s'"), *Pair.Value.NodeId.ToString()));
		}
	}
}


void FJ3DataValidator::CheckReachability(const FJ3GameData& Data, FJ3DataReport& Report)
{
	using namespace J3DataValidatorPrivate;

	// 어떤 레시피로도 만들지 않는 아이템은 월드에서 얻는 원자재로 본다
	TSet<FName> Produced;
	for (const TPair<FName, FJ3RecipeDef>& Pair : Data.Recipes)
	{
		Produced.Add(Pair.Value.Output.ItemId);
	}
	TSet<FName> Obtainable;
	for (const TPair<FName, FJ3ItemDef>& Pair : Data.Items)
	{
		if (!Produced.Contains(Pair.Key))
		{
			Obtainable.Add(Pair.Key);
		}
	}


	// 해방되고 재료를 모두 얻을 수 있는 레시피의 출력을 더 이상 늘지 않을 때까지 넓힌다.
	// 끝까지 얻을 수 없는 아이템은 해방 조건이 순환하거나 재료가 막힌 것이다.
	bool bChanged = true;
	while (bChanged)
	{
		bChanged = false;
		for (const TPair<FName, FJ3RecipeDef>& Pair : Data.Recipes)
		{
			const FJ3RecipeDef& Recipe = Pair.Value;
			if (Obtainable.Contains(Recipe.Output.ItemId) || !IsUnlockReachable(Recipe.Unlock, Obtainable))
			{
				continue;
			}
			const bool bInputsReady = Algo::AllOf(Recipe.Inputs, [&Obtainable](const FJ3ItemAmount& Input)
			{
				return Obtainable.Contains(Input.ItemId);
			});
			if (bInputsReady)
			{
				Obtainable.Add(Recipe.Output.ItemId);
				bChanged = true;
			}
		}
	}


	// 얻을 수 없는 아이템
	for (const TPair<FName, FJ3ItemDef>& Pair : Data.Items)
	{
		if (!Obtainable.Contains(Pair.Key))
		{
			Report.AddError(TEXT("items.csv"), LineOf(Data.ItemLines, Pair.Key), FString::Printf(TEXT("'%s'을(를) 얻을 방법이 없습니다 (해방 조건 순환 또는 재료 막힘)"), *Pair.Key.ToString()));
		}
	}


	// 해방될 수 없는 건물
	for (const TPair<FName, FJ3BuildingDef>& Pair : Data.Buildings)
	{
		if (!IsUnlockReachable(Pair.Value.Unlock, Obtainable))
		{
			Report.AddError(TEXT("buildings.csv"), LineOf(Data.BuildingLines, Pair.Key), FString::Printf(TEXT("'%s'의 해방 조건 아이템을 얻을 방법이 없습니다"), *Pair.Key.ToString()));
		}
	}
}


void FJ3DataValidator::WarnUndecided(const FJ3GameData& Data, FJ3DataReport& Report)
{
	// 미정 칸은 행마다 경고하면 로그가 묻히므로 표마다 한 줄로 모은다
	TArray<FString> NoStack;
	for (const TPair<FName, FJ3ItemDef>& Pair : Data.Items)
	{
		if (Pair.Value.StackSize <= 0)
		{
			NoStack.Add(Pair.Key.ToString());
		}
	}
	if (NoStack.Num() > 0)
	{
		Report.AddWarning(TEXT("items.csv"), 0, FString::Printf(TEXT("stack 미정 %d개 → config default_stack 사용: %s"), NoStack.Num(), *FString::Join(NoStack, TEXT(", "))));
	}


	TArray<FString> NoTime;
	for (const TPair<FName, FJ3RecipeDef>& Pair : Data.Recipes)
	{
		if (Pair.Value.TimeSec < 0.f)
		{
			NoTime.Add(Pair.Key.ToString());
		}
	}
	if (NoTime.Num() > 0)
	{
		Report.AddWarning(TEXT("recipes.csv"), 0, FString::Printf(TEXT("time_sec 미정 %d개 → 기본값 사용: %s"), NoTime.Num(), *FString::Join(NoTime, TEXT(", "))));
	}
}
