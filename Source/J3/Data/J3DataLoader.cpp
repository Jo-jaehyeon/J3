// Copyright (c) ACCENT. All Rights Reserved.

#include "Data/J3DataLoader.h"

#include "Data/J3CsvTable.h"
#include "Misc/DefaultValueHelper.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"


namespace J3DataLoaderPrivate
{
	// 한 행을 읽는 동안 오류 위치(파일:줄)를 자동으로 붙이기 위한 도우미
	struct FRowReader
	{
		const FJ3CsvTable& Table;
		const FJ3CsvTable::FRow& Row;
		FJ3DataReport& Report;
		bool bRowValid = true;

		void Error(const FString& Message)
		{
			Report.AddError(Table.GetFileName(), Row.Line, Message);
			bRowValid = false;
		}

		void Warning(const FString& Message)
		{
			Report.AddWarning(Table.GetFileName(), Row.Line, Message);
		}

		FString Optional(FName Column) const
		{
			return Table.Get(Row, Column);
		}

		FString Required(FName Column)
		{
			const FString Value = Table.Get(Row, Column);
			if (Value.IsEmpty())
			{
				Error(FString::Printf(TEXT("필수 칸 '%s'이(가) 비어 있습니다"), *Column.ToString()));
			}
			return Value;
		}

		int32 RequiredInt(FName Column)
		{
			const FString Value = Required(Column);
			int32 Result = 0;
			if (!Value.IsEmpty() && !FDefaultValueHelper::ParseInt(Value, Result))
			{
				Error(FString::Printf(TEXT("'%s' 값 '%s'은(는) 정수가 아닙니다"), *Column.ToString(), *Value));
			}
			return Result;
		}

		// 빈 칸이면 미정으로 보고 Fallback을 돌려준다
		int32 OptionalInt(FName Column, int32 Fallback)
		{
			const FString Value = Optional(Column);
			int32 Result = Fallback;
			if (!Value.IsEmpty() && !FDefaultValueHelper::ParseInt(Value, Result))
			{
				Error(FString::Printf(TEXT("'%s' 값 '%s'은(는) 정수가 아닙니다"), *Column.ToString(), *Value));
			}
			return Result;
		}

		float OptionalFloat(FName Column, float Fallback)
		{
			const FString Value = Optional(Column);
			float Result = Fallback;
			if (!Value.IsEmpty() && !FDefaultValueHelper::ParseFloat(Value, Result))
			{
				Error(FString::Printf(TEXT("'%s' 값 '%s'은(는) 숫자가 아닙니다"), *Column.ToString(), *Value));
			}
			return Result;
		}

		bool RequiredBool(FName Column)
		{
			const FString Value = Required(Column);
			if (Value.Equals(TEXT("true"), ESearchCase::IgnoreCase))
			{
				return true;
			}
			if (!Value.IsEmpty() && !Value.Equals(TEXT("false"), ESearchCase::IgnoreCase))
			{
				Error(FString::Printf(TEXT("'%s' 값 '%s'은(는) true/false가 아닙니다"), *Column.ToString(), *Value));
			}
			return false;
		}

		// "a|b|c"
		TArray<FName> RequiredNameList(FName Column)
		{
			TArray<FString> Parts;
			Required(Column).ParseIntoArray(Parts, TEXT("|"));
			TArray<FName> Names;
			for (const FString& Part : Parts)
			{
				Names.Add(FName(*Part.TrimStartAndEnd()));
			}
			return Names;
		}

		// "아이템:수량;아이템:수량"
		TArray<FJ3ItemAmount> RequiredItemAmounts(FName Column)
		{
			TArray<FString> Entries;
			Required(Column).ParseIntoArray(Entries, TEXT(";"));
			TArray<FJ3ItemAmount> Amounts;
			for (const FString& Entry : Entries)
			{
				FString ItemPart;
				FString CountPart;
				FJ3ItemAmount Amount;
				const bool bSplit = Entry.Split(TEXT(":"), &ItemPart, &CountPart);
				Amount.ItemId = FName(*ItemPart.TrimStartAndEnd());
				if (!bSplit || ItemPart.TrimStartAndEnd().IsEmpty() || !FDefaultValueHelper::ParseInt(CountPart.TrimStartAndEnd(), Amount.Count) || Amount.Count <= 0)
				{
					Error(FString::Printf(TEXT("'%s'의 '%s'은(는) '아이템:수량(1 이상)' 형식이 아닙니다"), *Column.ToString(), *Entry));
					continue;
				}
				Amounts.Add(Amount);
			}
			return Amounts;
		}

		// "auto" 또는 "on_obtain:a|b"
		FJ3UnlockRule RequiredUnlock(FName Column)
		{
			FJ3UnlockRule Rule;
			const FString Value = Required(Column);
			FString ItemsPart;
			if (Value.IsEmpty() || Value.Equals(TEXT("auto"), ESearchCase::IgnoreCase))
			{
				return Rule;
			}
			if (!Value.Split(TEXT("on_obtain:"), nullptr, &ItemsPart) || ItemsPart.TrimStartAndEnd().IsEmpty())
			{
				Error(FString::Printf(TEXT("'%s' 값 '%s'은(는) auto 또는 on_obtain:아이템 형식이 아닙니다"), *Column.ToString(), *Value));
				return Rule;
			}
			Rule.Type = EJ3UnlockType::OnObtain;
			TArray<FString> Parts;
			ItemsPart.ParseIntoArray(Parts, TEXT("|"));
			for (const FString& Part : Parts)
			{
				Rule.Items.Add(FName(*Part.TrimStartAndEnd()));
			}
			return Rule;
		}
	};


	// 표 하나를 열고 필수 열을 확인한다. 실패하면 행을 읽지 않는다.
	bool OpenTable(FJ3CsvTable& Table, const FString& DataDir, const TCHAR* FileName, const TArray<FName>& Columns, FJ3DataReport& Report, FString& OutRaw)
	{
		const FString Path = FPaths::Combine(DataDir, FileName);
		return Table.Load(Path, Report, OutRaw) && Table.RequireColumns(Columns, Report);
	}


	// 같은 표 안의 중복 ID를 막고 행 위치를 기록한다
	template <typename TDef>
	void AddUnique(TMap<FName, TDef>& Map, TMap<FName, int32>* Lines, TDef&& Def, FRowReader& Reader)
	{
		if (Map.Contains(Def.Id))
		{
			Reader.Error(FString::Printf(TEXT("중복 ID '%s'"), *Def.Id.ToString()));
			return;
		}
		if (!Reader.bRowValid)
		{
			return;
		}
		if (Lines != nullptr)
		{
			Lines->Add(Def.Id, Reader.Row.Line);
		}
		const FName Id = Def.Id;
		Map.Add(Id, MoveTemp(Def));
	}
}


bool FJ3DataLoader::LoadAll(const FString& DataDir, FJ3GameData& OutData, FJ3DataReport& Report)
{
	// 표 읽기. 표 하나가 실패해도 나머지 표의 오류까지 한 번에 보여 주려고 끝까지 읽는다.
	TArray<FString> RawContents;
	RawContents.SetNum(6);
	LoadItems(DataDir, OutData, Report, RawContents[0]);
	LoadRecipes(DataDir, OutData, Report, RawContents[1]);
	LoadBuildings(DataDir, OutData, Report, RawContents[2]);
	LoadConfig(DataDir, OutData, Report, RawContents[3]);
	LoadEquipment(DataDir, OutData, Report, RawContents[4]);
	LoadResourceNodes(DataDir, OutData, Report, RawContents[5]);


	// 컨베이어용 아이템 종류 번호
	OutData.Items.GenerateKeyArray(OutData.ItemTypeOrder);
	OutData.ItemTypeOrder.Sort(FNameLexicalLess());
	for (int32 Index = 0; Index < OutData.ItemTypeOrder.Num(); ++Index)
	{
		OutData.ItemTypeIds.Add(OutData.ItemTypeOrder[Index], static_cast<uint16>(Index + 1));
	}


	// 서버·클라 비교용 해시
	OutData.Hash = ComputeHash(RawContents);
	return !Report.HasErrors();
}


void FJ3DataLoader::LoadItems(const FString& DataDir, FJ3GameData& OutData, FJ3DataReport& Report, FString& OutRaw)
{
	using namespace J3DataLoaderPrivate;

	FJ3CsvTable Table;
	if (!OpenTable(Table, DataDir, TEXT("items.csv"), { "id", "name_ko", "tier", "category", "stack", "note" }, Report, OutRaw))
	{
		return;
	}


	for (const FJ3CsvTable::FRow& Row : Table.GetRows())
	{
		FRowReader Reader{ Table, Row, Report };
		FJ3ItemDef Def;
		Def.Id = FName(*Reader.Required("id"));
		Def.DisplayName = FText::FromString(Reader.Required("name_ko"));
		Def.Tier = Reader.RequiredInt("tier");
		Def.Category = FName(*Reader.Required("category"));
		Def.StackSize = Reader.OptionalInt("stack", 0);
		Def.Note = Reader.Optional("note");
		AddUnique(OutData.Items, &OutData.ItemLines, MoveTemp(Def), Reader);
	}
}


void FJ3DataLoader::LoadRecipes(const FString& DataDir, FJ3GameData& OutData, FJ3DataReport& Report, FString& OutRaw)
{
	using namespace J3DataLoaderPrivate;

	FJ3CsvTable Table;
	if (!OpenTable(Table, DataDir, TEXT("recipes.csv"), { "id", "output", "output_qty", "station", "inputs", "time_sec", "unlock" }, Report, OutRaw))
	{
		return;
	}


	for (const FJ3CsvTable::FRow& Row : Table.GetRows())
	{
		FRowReader Reader{ Table, Row, Report };
		FJ3RecipeDef Def;
		Def.Id = FName(*Reader.Required("id"));
		Def.Output.ItemId = FName(*Reader.Required("output"));
		Def.Output.Count = Reader.RequiredInt("output_qty");
		Def.Stations = Reader.RequiredNameList("station");
		Def.Inputs = Reader.RequiredItemAmounts("inputs");
		Def.TimeSec = Reader.OptionalFloat("time_sec", -1.f);
		Def.Unlock = Reader.RequiredUnlock("unlock");
		AddUnique(OutData.Recipes, &OutData.RecipeLines, MoveTemp(Def), Reader);
	}
}


void FJ3DataLoader::LoadBuildings(const FString& DataDir, FJ3GameData& OutData, FJ3DataReport& Report, FString& OutRaw)
{
	using namespace J3DataLoaderPrivate;

	FJ3CsvTable Table;
	if (!OpenTable(Table, DataDir, TEXT("buildings.csv"), { "id", "name_ko", "tier", "build_inputs", "needs_power", "unlock", "note" }, Report, OutRaw))
	{
		return;
	}


	for (const FJ3CsvTable::FRow& Row : Table.GetRows())
	{
		FRowReader Reader{ Table, Row, Report };
		FJ3BuildingDef Def;
		Def.Id = FName(*Reader.Required("id"));
		Def.DisplayName = FText::FromString(Reader.Required("name_ko"));
		Def.Tier = Reader.RequiredInt("tier");
		Def.BuildInputs = Reader.RequiredItemAmounts("build_inputs");
		Def.bNeedsPower = Reader.RequiredBool("needs_power");
		Def.Unlock = Reader.RequiredUnlock("unlock");
		Def.Note = Reader.Optional("note");
		AddUnique(OutData.Buildings, &OutData.BuildingLines, MoveTemp(Def), Reader);
	}
}


void FJ3DataLoader::LoadEquipment(const FString& DataDir, FJ3GameData& OutData, FJ3DataReport& Report, FString& OutRaw)
{
	using namespace J3DataLoaderPrivate;

	FJ3CsvTable Table;
	if (!OpenTable(Table, DataDir, TEXT("equipment.csv"), { "id", "ability", "damage", "range_cm", "cooldown_sec", "ammo", "harvest_tag", "harvest_multiplier" }, Report, OutRaw))
	{
		return;
	}


	for (const FJ3CsvTable::FRow& Row : Table.GetRows())
	{
		FRowReader Reader{ Table, Row, Report };
		FJ3EquipmentDef Def;
		Def.Id = FName(*Reader.Required("id"));
		Def.Ability = FSoftClassPath(Reader.Optional("ability"));
		Def.Damage = Reader.OptionalFloat("damage", 0.f);
		Def.RangeCm = Reader.OptionalFloat("range_cm", 0.f);
		Def.CooldownSec = Reader.OptionalFloat("cooldown_sec", -1.f);
		Def.AmmoItem = FName(*Reader.Optional("ammo"));
		Def.HarvestTag = FName(*Reader.Optional("harvest_tag"));
		Def.HarvestMultiplier = Reader.OptionalFloat("harvest_multiplier", 1.f);
		AddUnique(OutData.Equipment, &OutData.EquipmentLines, MoveTemp(Def), Reader);
	}
}


void FJ3DataLoader::LoadResourceNodes(const FString& DataDir, FJ3GameData& OutData, FJ3DataReport& Report, FString& OutRaw)
{
	using namespace J3DataLoaderPrivate;

	FJ3CsvTable Table;
	if (!OpenTable(Table, DataDir, TEXT("resource_nodes.csv"), { "id", "name_ko", "item", "yield", "harvest_sec", "tool_tag", "required_tag" }, Report, OutRaw))
	{
		return;
	}


	for (const FJ3CsvTable::FRow& Row : Table.GetRows())
	{
		FRowReader Reader{ Table, Row, Report };
		FJ3ResourceNodeDef Def;
		Def.Id = FName(*Reader.Required("id"));
		Def.DisplayName = FText::FromString(Reader.Required("name_ko"));
		Def.ItemId = FName(*Reader.Required("item"));
		Def.Yield = Reader.RequiredInt("yield");
		Def.HarvestSec = Reader.OptionalFloat("harvest_sec", -1.f);
		Def.ToolTag = FName(*Reader.Optional("tool_tag"));
		Def.RequiredTag = FName(*Reader.Optional("required_tag"));
		if (Def.Yield <= 0 || Def.HarvestSec <= 0.f)
		{
			Reader.Error(TEXT("yield와 harvest_sec는 0보다 커야 합니다"));
		}
		AddUnique(OutData.ResourceNodes, &OutData.ResourceNodeLines, MoveTemp(Def), Reader);
	}
}


void FJ3DataLoader::LoadConfig(const FString& DataDir, FJ3GameData& OutData, FJ3DataReport& Report, FString& OutRaw)
{
	using namespace J3DataLoaderPrivate;

	FJ3CsvTable Table;
	if (!OpenTable(Table, DataDir, TEXT("config.csv"), { "id", "value" }, Report, OutRaw))
	{
		return;
	}


	for (const FJ3CsvTable::FRow& Row : Table.GetRows())
	{
		// config는 ID → 숫자 하나라서 별도 구조체 없이 바로 넣는다
		FRowReader Reader{ Table, Row, Report };
		const FName Id = FName(*Reader.Required("id"));
		const FString ValueText = Reader.Required("value");
		float Value = 0.f;
		if (!ValueText.IsEmpty() && !FDefaultValueHelper::ParseFloat(ValueText, Value))
		{
			Reader.Error(FString::Printf(TEXT("'%s'의 value가 숫자가 아닙니다"), *Id.ToString()));
		}
		if (OutData.Config.Contains(Id))
		{
			Reader.Error(FString::Printf(TEXT("중복 ID '%s'"), *Id.ToString()));
		}
		if (Reader.bRowValid)
		{
			OutData.Config.Add(Id, Value);
		}
	}
}


FString FJ3DataLoader::ComputeHash(const TArray<FString>& RawContents)
{
	// 줄바꿈을 LF로 맞춰 git 설정(CRLF/LF)이 다른 PC끼리도 같은 해시가 나오게 한다
	FMD5 Md5;
	for (const FString& Raw : RawContents)
	{
		const FString Normalized = Raw.Replace(TEXT("\r\n"), TEXT("\n"));
		const FTCHARToUTF8 Utf8(*Normalized);
		Md5.Update(reinterpret_cast<const uint8*>(Utf8.Get()), Utf8.Length());
	}
	uint8 Digest[16];
	Md5.Final(Digest);
	return BytesToHex(Digest, UE_ARRAY_COUNT(Digest));
}
