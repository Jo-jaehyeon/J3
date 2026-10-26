// Copyright (c) ACCENT. All Rights Reserved.

#include "Factory/J3FactoryReplicator.h"

#include "Data/J3DataSubsystem.h"
#include "Engine/World.h"
#include "Factory/J3FactorySimSubsystem.h"
#include "Net/UnrealNetwork.h"


void FJ3SegmentNetItem::PostReplicatedAdd(const FJ3SegmentNetArray& Array)
{
	if (Array.Owner != nullptr)
	{
		Array.Owner->ApplySegment(*this);
	}
}


void FJ3SegmentNetItem::PostReplicatedChange(const FJ3SegmentNetArray& Array)
{
	if (Array.Owner != nullptr)
	{
		Array.Owner->ApplySegment(*this);
	}
}


void FJ3SegmentNetItem::PreReplicatedRemove(const FJ3SegmentNetArray& Array)
{
	if (Array.Owner != nullptr)
	{
		Array.Owner->RemoveSegment(SegmentId);
	}
}


AJ3FactoryReplicator::AJ3FactoryReplicator()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	bAlwaysRelevant = true;
	Segments.Owner = this;
}


void AJ3FactoryReplicator::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AJ3FactoryReplicator, Segments);
}


void AJ3FactoryReplicator::BeginPlay()
{
	Super::BeginPlay();
	if (!HasAuthority())
	{
		return;
	}


	// 서버: 시뮬레이션의 변경 이벤트를 받아 보낼 구간을 모은다
	ResyncIntervalSec = UJ3DataSubsystem::Get()->GetConfigFloat("factory_resync_sec", 0.f);
	UJ3FactorySimSubsystem* Simulation = GetWorld()->GetSubsystem<UJ3FactorySimSubsystem>();
	SegmentsChangedHandle = Simulation->OnSegmentsChanged.AddUObject(this, &AJ3FactoryReplicator::OnSegmentsChanged);
	SyncTopology();
}


void AJ3FactoryReplicator::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UJ3FactorySimSubsystem* Simulation = GetWorld()->GetSubsystem<UJ3FactorySimSubsystem>())
	{
		Simulation->OnSegmentsChanged.Remove(SegmentsChangedHandle);
	}
	Super::EndPlay(EndPlayReason);
}


void AJ3FactoryReplicator::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!HasAuthority())
	{
		return;
	}
	const UJ3FactorySimSubsystem* Simulation = GetWorld()->GetSubsystem<UJ3FactorySimSubsystem>();
	if (SyncedTopologyVersion != Simulation->GetTopologyVersion())
	{
		SyncTopology();
	}
	SendPendingSegments();
}


void AJ3FactoryReplicator::CopyToNet(const FJ3BeltSegment& Segment, FJ3SegmentNetItem& OutItem)
{
	// 구간 정보 + 아이템 (위치는 mm로 줄여 4바이트)
	OutItem.SegmentId = Segment.Id;
	OutItem.NextId = Segment.NextId;
	OutItem.LengthCm = Segment.LengthCm;
	OutItem.SpeedCmPerSec = Segment.SpeedCmPerSec;
	OutItem.SpacingCm = Segment.SpacingCm;
	OutItem.StartLocation = Segment.StartLocation;
	OutItem.EndLocation = Segment.EndLocation;
	OutItem.Items.SetNum(Segment.Items.Num());
	for (int32 Index = 0; Index < Segment.Items.Num(); ++Index)
	{
		OutItem.Items[Index].OffsetMm = static_cast<uint16>(FMath::Clamp(FMath::RoundToInt(Segment.Items[Index].Offset * 10.f), 0, MAX_uint16));
		OutItem.Items[Index].Type = Segment.Items[Index].Type;
	}
}


void AJ3FactoryReplicator::SyncTopology()
{
	const UJ3FactorySimSubsystem* Simulation = GetWorld()->GetSubsystem<UJ3FactorySimSubsystem>();

	// 없어진 구간 빼기
	const int32 RemovedCount = Segments.Items.RemoveAll([Simulation](const FJ3SegmentNetItem& Item)
	{
		return Simulation->FindSegment(Item.SegmentId) == nullptr;
	});
	if (RemovedCount > 0)
	{
		Segments.MarkArrayDirty();
	}
	NetIndexById.Reset();
	for (int32 Index = 0; Index < Segments.Items.Num(); ++Index)
	{
		NetIndexById.Add(Segments.Items[Index].SegmentId, Index);
	}


	// 새 구간 넣기 (바로 보낸다)
	for (const FJ3BeltSegment& Segment : Simulation->GetSegments())
	{
		if (NetIndexById.Contains(Segment.Id))
		{
			continue;
		}
		FJ3SegmentNetItem& NetItem = Segments.Items.AddDefaulted_GetRef();
		CopyToNet(Segment, NetItem);
		Segments.MarkItemDirty(NetItem);
		NetIndexById.Add(Segment.Id, Segments.Items.Num() - 1);
		LastSentTime.Add(Segment.Id, GetWorld()->GetTimeSeconds());
	}
	SyncedTopologyVersion = Simulation->GetTopologyVersion();
}


void AJ3FactoryReplicator::OnSegmentsChanged(const TSet<int32>& SegmentIds)
{
	PendingSegmentIds.Append(SegmentIds);
}


void AJ3FactoryReplicator::SendPendingSegments()
{
	// 구간마다 보정 주기(factory_resync_sec)보다 자주 보내지 않는다. 그 사이는 클라이언트가 같은 규칙으로 움직인다.
	const UJ3FactorySimSubsystem* Simulation = GetWorld()->GetSubsystem<UJ3FactorySimSubsystem>();
	const double Now = GetWorld()->GetTimeSeconds();
	for (auto It = PendingSegmentIds.CreateIterator(); It; ++It)
	{
		const int32 SegmentId = *It;
		const double* LastSent = LastSentTime.Find(SegmentId);
		if (LastSent != nullptr && Now - *LastSent < ResyncIntervalSec)
		{
			continue;
		}
		const FJ3BeltSegment* Segment = Simulation->FindSegment(SegmentId);
		const int32* NetIndex = NetIndexById.Find(SegmentId);
		if (Segment != nullptr && NetIndex != nullptr)
		{
			CopyToNet(*Segment, Segments.Items[*NetIndex]);
			Segments.MarkItemDirty(Segments.Items[*NetIndex]);
			LastSentTime.Add(SegmentId, Now);
		}
		It.RemoveCurrent();
	}
}


void AJ3FactoryReplicator::ApplySegment(const FJ3SegmentNetItem& NetItem)
{
	// 클라이언트: 서버 값으로 덮어쓴다 (구간이 없으면 새로 만든다)
	UJ3FactorySimSubsystem* Simulation = GetWorld() != nullptr ? GetWorld()->GetSubsystem<UJ3FactorySimSubsystem>() : nullptr;
	if (Simulation == nullptr || HasAuthority())
	{
		return;
	}
	FJ3BeltSegment Segment;
	Segment.Id = NetItem.SegmentId;
	Segment.NextId = NetItem.NextId;
	Segment.LengthCm = NetItem.LengthCm;
	Segment.SpeedCmPerSec = NetItem.SpeedCmPerSec;
	Segment.SpacingCm = NetItem.SpacingCm;
	Segment.StartLocation = NetItem.StartLocation;
	Segment.EndLocation = NetItem.EndLocation;
	Segment.Items.SetNum(NetItem.Items.Num());
	for (int32 Index = 0; Index < NetItem.Items.Num(); ++Index)
	{
		Segment.Items[Index].Offset = NetItem.Items[Index].OffsetMm * 0.1f;
		Segment.Items[Index].Type = NetItem.Items[Index].Type;
	}


	// 이미 있는 구간이면 그 자리에 덮어써 표시 쪽이 컨베이어 모양을 다시 만들지 않게 한다
	if (FJ3BeltSegment* Existing = Simulation->FindSegmentMutable(Segment.Id))
	{
		*Existing = MoveTemp(Segment);
		return;
	}
	Simulation->AddSegment(Segment);
}


void AJ3FactoryReplicator::RemoveSegment(int32 SegmentId)
{
	UJ3FactorySimSubsystem* Simulation = GetWorld() != nullptr ? GetWorld()->GetSubsystem<UJ3FactorySimSubsystem>() : nullptr;
	if (Simulation != nullptr && !HasAuthority())
	{
		Simulation->RemoveSegment(SegmentId);
	}
}
