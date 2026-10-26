// Copyright (c) ACCENT. All Rights Reserved.

#include "Building/J3BuildingTypes.h"

#define LOCTEXT_NAMESPACE "J3Building"


namespace J3Building
{
	FText GetResultText(EJ3PlacementResult Result)
	{
		switch (Result)
		{
		case EJ3PlacementResult::Ok:
			return LOCTEXT("Ok", "설치할 수 있습니다");
		case EJ3PlacementResult::UnknownBuilding:
			return LOCTEXT("Unknown", "알 수 없는 건물입니다");
		case EJ3PlacementResult::NotUnlocked:
			return LOCTEXT("NotUnlocked", "아직 해방되지 않은 건물입니다");
		case EJ3PlacementResult::MissingMaterials:
			return LOCTEXT("MissingMaterials", "재료가 부족합니다");
		case EJ3PlacementResult::AtLimit:
			return LOCTEXT("AtLimit", "이 건물은 더 지을 수 없습니다");
		case EJ3PlacementResult::AtScaleCap:
			return LOCTEXT("AtScaleCap", "더 이상 설치할 수 없습니다");
		case EJ3PlacementResult::TooFar:
			return LOCTEXT("TooFar", "너무 멉니다");
		case EJ3PlacementResult::NoGround:
			return LOCTEXT("NoGround", "놓을 바닥이 없습니다");
		case EJ3PlacementResult::NeedsResourceNode:
			return LOCTEXT("NeedsResourceNode", "맞는 자원 위에만 설치할 수 있습니다");
		case EJ3PlacementResult::TooSteep:
			return LOCTEXT("TooSteep", "경사가 너무 급합니다 (토대 위에 설치하세요)");
		case EJ3PlacementResult::Overlapping:
			return LOCTEXT("Overlapping", "다른 물체와 겹칩니다");
		}
		return FText::GetEmpty();
	}


	FVector QuantizeLocation(const FVector& Location)
	{
		return FVector(FMath::RoundToDouble(Location.X), FMath::RoundToDouble(Location.Y), FMath::RoundToDouble(Location.Z));
	}


	float QuantizeYaw(float YawDeg)
	{
		return FRotator::NormalizeAxis(FMath::RoundToFloat(YawDeg));
	}
}

#undef LOCTEXT_NAMESPACE
