#pragma once

#include "CoreMinimal.h"
#include "Data/J3DataTypes.h"

struct FJ3DataReport;


// 표 사이의 관계를 검사한다: 없는 아이템 참조, 해방 조건 순환(얻을 방법이 없는 아이템), 미정 칸 경고.
class FJ3DataValidator
{
public:
	static void Validate(const FJ3GameData& Data, FJ3DataReport& Report);

private:
	static void CheckReferences(const FJ3GameData& Data, FJ3DataReport& Report);
	static void CheckReachability(const FJ3GameData& Data, FJ3DataReport& Report);
	static void WarnUndecided(const FJ3GameData& Data, FJ3DataReport& Report);
};
