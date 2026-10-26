// Copyright (c) ACCENT. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "Factory/J3BeltTypes.h"
#include "J3FactoryReplicator.generated.h"

class AJ3FactoryReplicator;
struct FJ3SegmentNetArray;


// 네트워크로 보내는 컨베이어 아이템 (4바이트: 위치 mm, 종류)
USTRUCT()
struct FJ3BeltItemNet
{
	GENERATED_BODY()

	// 구간 시작에서 mm. 구간 길이는 65.5m를 넘지 않는다.
	UPROPERTY()
	uint16 OffsetMm = 0;

	UPROPERTY()
	uint16 Type = 0;
};


// 구간 하나의 복제 단위. 위치를 매 틱 보내지 않고, 아이템이 들어오거나 나갈 때(또는 보정 주기마다) 구간 내용을 보낸다.
// 클라이언트는 받은 내용으로 자기 시뮬레이션의 구간을 덮어쓰고, 그 사이에는 같은 이동 규칙으로 직접 움직인다.
USTRUCT()
struct FJ3SegmentNetItem : public FFastArraySerializerItem
{
	GENERATED_BODY()

	UPROPERTY()
	int32 SegmentId = INDEX_NONE;

	UPROPERTY()
	int32 NextId = INDEX_NONE;

	UPROPERTY()
	float LengthCm = 0.f;

	UPROPERTY()
	float SpeedCmPerSec = 0.f;

	UPROPERTY()
	float SpacingCm = 0.f;

	UPROPERTY()
	FVector_NetQuantize StartLocation;

	UPROPERTY()
	FVector_NetQuantize EndLocation;

	UPROPERTY()
	TArray<FJ3BeltItemNet> Items;

	void PostReplicatedAdd(const FJ3SegmentNetArray& Array);
	void PostReplicatedChange(const FJ3SegmentNetArray& Array);
	void PreReplicatedRemove(const FJ3SegmentNetArray& Array);
};


USTRUCT()
struct FJ3SegmentNetArray : public FFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FJ3SegmentNetItem> Items;

	// 받은 내용을 반영할 곳 (복제하지 않음)
	UPROPERTY(NotReplicated)
	TObjectPtr<AJ3FactoryReplicator> Owner;

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParams)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FJ3SegmentNetItem, FJ3SegmentNetArray>(Items, DeltaParams, *this);
	}
};

template<>
struct TStructOpsTypeTraits<FJ3SegmentNetArray> : public TStructOpsTypeTraitsBase2<FJ3SegmentNetArray>
{
	enum
	{
		WithNetDeltaSerializer = true,
	};
};


// 공장 시뮬레이션 상태를 클라이언트로 보내는 액터. 공간 청크(격자 칸)마다 하나씩 서버가 만든다.
// Iris는 액터 하나가 한 번에 보낼 수 있는 양에 상한(512KB)이 있어, 한 액터에 모든 구간을 담으면 큰 공장에서 중도 참가가 끊긴다 (단계 2 측정).
// 중도 참가자는 접속할 때 청크별로 전체 구간을 처음 복제로 받는다 (Multiplayer_Architecture 4.3 청크 단위 스냅샷).
UCLASS(NotPlaceable)
class J3_API AJ3FactoryReplicator : public AActor
{
	GENERATED_BODY()

public:
	AJ3FactoryReplicator();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;


	// 서버: 이 청크에 속한 구간 목록을 맞춘다 (새 구간은 바로 보내고, 없어진 구간은 뺀다)
	void SetSegments(const TArray<int32>& SegmentIds);

	// 서버: 아이템이 들어오거나 나간 구간 (보정 주기에 맞춰 보낸다)
	void MarkSegmentDirty(int32 SegmentId);


	// 클라이언트: 받은 구간을 자기 시뮬레이션에 반영
	void ApplySegment(const FJ3SegmentNetItem& NetItem);
	void RemoveSegment(int32 SegmentId);

private:
	void SendPendingSegments();
	void RebuildNetIndex();
	static void CopyToNet(const FJ3BeltSegment& Segment, FJ3SegmentNetItem& OutItem);

	UPROPERTY(Replicated)
	FJ3SegmentNetArray Segments;

	// 서버 전용: 보내야 할 구간과 마지막으로 보낸 시각 (보정 주기 제한)
	TSet<int32> PendingSegmentIds;
	TMap<int32, double> LastSentTime;
	TMap<int32, int32> NetIndexById;
	float ResyncIntervalSec = 0.f;
};
