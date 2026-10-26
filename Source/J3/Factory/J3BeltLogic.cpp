// Copyright (c) ACCENT. All Rights Reserved.

#include "Factory/J3BeltLogic.h"


namespace J3BeltLogicPrivate
{
	// 끝에 닿아 다음 구간으로 넘어가려는 아이템 (넘긴 거리 포함)
	struct FPendingTransfer
	{
		int32 SegmentIndex = INDEX_NONE;
		int32 NextIndex = INDEX_NONE;
		float OverflowCm = 0.f;
	};
}


namespace J3BeltLogic
{
	bool HasRoomAtStart(const FJ3BeltSegment& Segment)
	{
		return Segment.Items.Num() == 0 || Segment.Items.Last().Offset >= Segment.SpacingCm;
	}


	bool TryInsert(FJ3BeltSegment& Segment, FJ3ItemTypeId Type)
	{
		if (!HasRoomAtStart(Segment))
		{
			return false;
		}
		FJ3BeltItem Item;
		Item.Type = Type;
		Item.Offset = 0.f;
		Segment.Items.Add(Item);
		return true;
	}


	void Step(TArray<FJ3BeltSegment>& Segments, const TMap<int32, int32>& IndexById, float DeltaSec, TSet<int32>& OutChangedSegments)
	{
		using namespace J3BeltLogicPrivate;

		// 1단계: 각 구간 안에서 이동. 맨 앞은 끝까지, 나머지는 앞 아이템과 간격을 유지한다.
		// 넘김을 따로 처리해야 넘어간 아이템이 같은 틱에 다음 구간에서 한 번 더 움직이지 않는다.
		TArray<FPendingTransfer> Transfers;
		for (int32 SegmentIndex = 0; SegmentIndex < Segments.Num(); ++SegmentIndex)
		{
			FJ3BeltSegment& Segment = Segments[SegmentIndex];
			const float Move = Segment.SpeedCmPerSec * DeltaSec;
			for (int32 Index = 0; Index < Segment.Items.Num(); ++Index)
			{
				FJ3BeltItem& Item = Segment.Items[Index];
				const float Target = Item.Offset + Move;
				if (Index > 0)
				{
					const float Limit = Segment.Items[Index - 1].Offset - Segment.SpacingCm;
					Item.Offset = FMath::Max(Item.Offset, FMath::Min(Target, Limit));
					continue;
				}
				Item.Offset = FMath::Min(Target, Segment.LengthCm);
				const int32* NextIndex = IndexById.Find(Segment.NextId);
				if (Target >= Segment.LengthCm && NextIndex != nullptr)
				{
					Transfers.Add({ SegmentIndex, *NextIndex, Target - Segment.LengthCm });
				}
			}
		}


		// 2단계: 끝에 닿은 맨 앞 아이템을 다음 구간으로 넘긴다. 다음 구간 시작에 자리가 없으면 끝에서 기다린다.
		for (const FPendingTransfer& Transfer : Transfers)
		{
			FJ3BeltSegment& Segment = Segments[Transfer.SegmentIndex];
			FJ3BeltSegment& Next = Segments[Transfer.NextIndex];
			if (Segment.Items.Num() == 0 || !HasRoomAtStart(Next))
			{
				continue;
			}
			FJ3BeltItem Moved = Segment.Items[0];
			const float RearLimit = Next.Items.Num() > 0 ? Next.Items.Last().Offset - Next.SpacingCm : Next.LengthCm;
			Moved.Offset = FMath::Clamp(Transfer.OverflowCm, 0.f, RearLimit);
			Next.Items.Add(Moved);
			Segment.Items.RemoveAt(0);
			OutChangedSegments.Add(Segment.Id);
			OutChangedSegments.Add(Next.Id);
		}
	}


	int32 CountItems(const TArray<FJ3BeltSegment>& Segments)
	{
		int32 Total = 0;
		for (const FJ3BeltSegment& Segment : Segments)
		{
			Total += Segment.Items.Num();
		}
		return Total;
	}
}
