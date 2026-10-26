// Copyright (c) ACCENT. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Building/J3BuildingTypes.h"
#include "J3BuildModeComponent.generated.h"

class AJ3BuildPreview;
class AJ3BuildingBase;


// 건설 모드 (Inventory_And_UI 1.3, 이 PC 전용): 건물을 고르면 조준한 곳에 미리보기를 띄우고,
// 같은 판정 함수로 파랑/빨강과 불가 사유를 보여 준다. 좌클릭으로 설치 요청, 휠로 회전, 우클릭·Esc로 끝낸다.
// 숫자키 단축 칸(1~0)에 건물을 등록해 바로 들어갈 수 있고, 가운데 클릭으로 조준한 건물과 같은 건물을 고른다.
// 철거 모드(F): 조준한 건물을 빨간 모형으로 표시하고 환급 재료를 보여 주며, 좌클릭을 길게 누르면 철거를 요청한다.
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

	bool IsDemolishing() const
	{
		return bDemolishing;
	}

	// 설치든 철거든 건설 모드 중 (휠·우클릭이 장비 대신 건설 조작으로 쓰인다)
	bool IsActive() const
	{
		return IsPlacing() || IsDemolishing();
	}

	// 건설 모드를 모두 끝낸다 (우클릭·Esc)
	void EndAll()
	{
		EndPlacing();
		EndDemolish();
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

	// 보조 그리드 켜기/끄기 (Building_Placement 2.1, 켜면 위치는 격자, 회전은 회전 단위에 맞춘다)
	void ToggleGrid()
	{
		bGrid = !bGrid;
	}

	bool IsGridEnabled() const
	{
		return bGrid;
	}


	// F: 철거 모드 켜기/끄기 (Inventory_And_UI 1.3)
	void ToggleDemolish();
	void EndDemolish();

	// 좌클릭 누름/뗌. 설치 모드면 누를 때 설치, 철거 모드면 누르고 있는 동안 진행
	void PressPrimary();
	void ReleasePrimary();

	// 철거 대상 (조준 중인 건물, 없으면 null)과 사거리 안인지
	const AJ3BuildingBase* GetDemolishTarget() const
	{
		return DemolishTarget.Get();
	}

	bool IsDemolishTargetInRange() const
	{
		return bDemolishTargetInRange;
	}

	// 길게 누르기 진행도 0~1, 누르고 있지 않으면 -1 (HUD 막대)
	float GetDemolishProgress() const;


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
	void UpdateDemolish(float DeltaTime);

	FName SelectedBuildingId;
	float YawDeg = 0.f;
	bool bGrid = false;
	FJ3PlacementCheck LastCheck;
	FVector LastAimLocation = FVector::ZeroVector;

	UPROPERTY(Transient)
	TObjectPtr<AJ3BuildPreview> Preview;

	// 철거 모드: 대상 위에 겹쳐 그리는 빨간 모형
	UPROPERTY(Transient)
	TObjectPtr<AJ3BuildPreview> DemolishHighlight;

	bool bDemolishing = false;
	bool bPrimaryHeld = false;
	bool bDemolishTargetInRange = false;
	float DemolishHoldSec = 0.f;
	TWeakObjectPtr<AJ3BuildingBase> DemolishTarget;

	// 요청을 보낸 건물 (서버에서 사라질 때까지 다시 보내지 않는다)
	TWeakObjectPtr<AJ3BuildingBase> RequestedTarget;

	// TODO(단계 8): 단축 칸을 개인 프로필에 저장
	TArray<FName> HotbarSlots;
};
