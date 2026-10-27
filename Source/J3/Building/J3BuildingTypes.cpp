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
		case EJ3PlacementResult::ConveyorTooShort:
			return LOCTEXT("ConveyorTooShort", "컨베이어가 너무 짧습니다");
		case EJ3PlacementResult::ConveyorTooLong:
			return LOCTEXT("ConveyorTooLong", "컨베이어가 너무 깁니다");
		case EJ3PlacementResult::ConveyorTooSharp:
			return LOCTEXT("ConveyorTooSharp", "너무 급하게 꺾입니다");
		case EJ3PlacementResult::PortInUse:
			return LOCTEXT("PortInUse", "이미 컨베이어가 연결된 포트입니다");
		case EJ3PlacementResult::NeedsOutputPort:
			return LOCTEXT("NeedsOutputPort", "기계의 출구(나가는 포트)를 조준하세요");
		case EJ3PlacementResult::NeedsInputPort:
			return LOCTEXT("NeedsInputPort", "기계·창고의 입구(들어오는 포트)를 조준하세요");
		case EJ3PlacementResult::NeedsPowerNode:
			return LOCTEXT("NeedsPowerNode", "발전소·전신주·전기를 쓰는 기계를 조준하세요");
		case EJ3PlacementResult::WireExists:
			return LOCTEXT("WireExists", "이미 이어져 있습니다");
		case EJ3PlacementResult::WireTooLong:
			return LOCTEXT("WireTooLong", "전기줄이 너무 깁니다 (멀리는 전신주로)");
		case EJ3PlacementResult::WireLimit:
			return LOCTEXT("WireLimit", "더 이을 수 없습니다 (전기를 쓰는 기계 1개, 전신주 4개)");
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
