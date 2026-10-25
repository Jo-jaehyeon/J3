#pragma once

#include "CoreMinimal.h"
#include "Data/J3DataTypes.h"

struct FJ3DataReport;


// Data/ 폴더의 CSV 원본을 읽어 FJ3GameData로 만든다. 표 안의 형식 오류와 중복 ID까지 잡고,
// 표 사이의 참조 검사는 FJ3DataValidator가 맡는다.
class FJ3DataLoader
{
public:
	static bool LoadAll(const FString& DataDir, FJ3GameData& OutData, FJ3DataReport& Report);

private:
	static void LoadItems(const FString& DataDir, FJ3GameData& OutData, FJ3DataReport& Report, FString& OutRaw);
	static void LoadRecipes(const FString& DataDir, FJ3GameData& OutData, FJ3DataReport& Report, FString& OutRaw);
	static void LoadBuildings(const FString& DataDir, FJ3GameData& OutData, FJ3DataReport& Report, FString& OutRaw);
	static void LoadConfig(const FString& DataDir, FJ3GameData& OutData, FJ3DataReport& Report, FString& OutRaw);

	static FString ComputeHash(const TArray<FString>& RawContents);
};
