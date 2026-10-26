// Copyright (c) ACCENT. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Building/J3BuildingTypes.h"
#include "J3BuildModeComponent.generated.h"

class AJ3BuildPreview;


// 건설 모드 (Inventory_And_UI 1.3, 이 PC 전용): 건물을 고르면 조준한 곳에 미리보기를 띄우고,
// 같은 판정 함수로 파랑/빨강과 불가 사유를 보여 준다. 좌클릭으로 설치 요청, 휠로 회전, 우클릭·Esc로 끝낸다.
// 숫자키 단축 칸(1~0)에 건물을 등록해 바로 들어갈 수 있고, 가운데 클릭으로 조준한 건물과 같은 건물을 고른다.
// 확정은 서버(UJ3BuilderComponent)가 한다.
UCLASS(ClassGroup = (J3))
class J3_API UJ3BuildModeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UJ3BuildModeComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	bool IsPlacing() const
	{
		return !SelectedBuildingId.IsNone();
	}

	FName GetSelectedBuildingId() const
	{
		return SelectedBuildingId;
	}

	const FJ3PlacementCheck& GetLastCheck() const
	{
		return LastCheck;
	}


	void BeginPlacing(FName BuildingId);
	void EndPlacing();

	// 휠 한 칸 회전 (건설 모드에서 휠은 장비 변경 대신 회전)
	void Rotate(int32 Direction);

	// 좌클릭: 지금 미리보기 위치에 설치 요청 (모드는 유지해 연달아 지을 수 있다)
	void Confirm();

	// 가운데 클릭: 조준한 건물과 같은 건물·방향으로 시작
	void PickAimedBuilding();


	// 숫자키 단축 칸 (1~0 → 0~9)
	void AssignHotbar(int32 Index, FName BuildingId);
	void UseHotbar(int32 Index);

	FName GetHotbar(int32 Index) const
	{
		return HotbarSlots.IsValidIndex(Index) ? HotbarSlots[Index] : NAME_None;
	}

private:
	bool TraceAim(FHitResult& OutHit) const;
	void UpdatePreview();

	FName SelectedBuildingId;
	float YawDeg = 0.f;
	FJ3PlacementCheck LastCheck;
	FVector LastAimLocation = FVector::ZeroVector;

	UPROPERTY(Transient)
	TObjectPtr<AJ3BuildPreview> Preview;

	// TODO(단계 8): 단축 칸을 개인 프로필에 저장
	TArray<FName> HotbarSlots;
};
