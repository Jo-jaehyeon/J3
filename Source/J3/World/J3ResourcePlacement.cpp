// Copyright (c) ACCENT. All Rights Reserved.

#include "World/J3ResourcePlacement.h"


namespace J3ResourcePlacementPrivate
{
	bool IsFarEnough(const TArray<J3ResourcePlacement::FPlacedNode>& Placed, const FVector2D& Candidate, float MinSpacing)
	{
		const float MinSpacingSquared = MinSpacing * MinSpacing;
		for (const J3ResourcePlacement::FPlacedNode& Node : Placed)
		{
			if (FVector2D::DistSquared(Node.Location, Candidate) < MinSpacingSquared)
			{
				return false;
			}
		}
		return true;
	}


	// 원 안 균일 분포 (반지름에 제곱근을 써서 가운데로 몰리지 않게)
	FVector2D RandomInDisk(FRandomStream& Stream, const FVector2D& Center, float Radius)
	{
		const float Angle = Stream.FRandRange(0.f, 2.f * PI);
		const float Distance = Radius * FMath::Sqrt(Stream.FRand());
		return Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Distance;
	}


	FVector2D RandomInBox(FRandomStream& Stream, const FVector2D& Center, const FVector2D& HalfExtent)
	{
		return Center + FVector2D(Stream.FRandRange(-HalfExtent.X, HalfExtent.X), Stream.FRandRange(-HalfExtent.Y, HalfExtent.Y));
	}


	bool IsInsideArea(const J3ResourcePlacement::FSettings& Settings, const FVector2D& Location)
	{
		const FVector2D Offset = (Location - Settings.AreaCenter).GetAbs();
		return Offset.X <= Settings.AreaHalfExtent.X && Offset.Y <= Settings.AreaHalfExtent.Y;
	}
}


namespace J3ResourcePlacement
{
	TArray<FPlacedNode> Place(const FSettings& Settings, const TArray<FJ3ResourceSpawnDef>& Rules)
	{
		using namespace J3ResourcePlacementPrivate;

		FRandomStream Stream(Settings.Seed);
		TArray<FPlacedNode> Placed;


		// 1단계: 보장 개수를 시작 지점 반경 안(구역 안)에 먼저 놓는다. 규칙 순서는 호출하는 쪽이 고정한다.
		for (const FJ3ResourceSpawnDef& Rule : Rules)
		{
			for (int32 Index = 0; Index < Rule.Guarantee; ++Index)
			{
				for (int32 Attempt = 0; Attempt < Settings.MaxAttempts; ++Attempt)
				{
					const FVector2D Candidate = RandomInDisk(Stream, Settings.GuaranteeCenter, Settings.GuaranteeRadius);
					if (IsInsideArea(Settings, Candidate) && IsFarEnough(Placed, Candidate, Settings.MinSpacing))
					{
						Placed.Add({ Rule.NodeId, Candidate, true });
						break;
					}
				}
			}
		}


		// 2단계: 나머지를 구역 전체에 흩는다
		for (const FJ3ResourceSpawnDef& Rule : Rules)
		{
			for (int32 Index = Rule.Guarantee; Index < Rule.Count; ++Index)
			{
				for (int32 Attempt = 0; Attempt < Settings.MaxAttempts; ++Attempt)
				{
					const FVector2D Candidate = RandomInBox(Stream, Settings.AreaCenter, Settings.AreaHalfExtent);
					if (IsFarEnough(Placed, Candidate, Settings.MinSpacing))
					{
						Placed.Add({ Rule.NodeId, Candidate, false });
						break;
					}
				}
			}
		}
		return Placed;
	}
}
