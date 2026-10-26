// Copyright (c) ACCENT. All Rights Reserved.

#include "Building/J3BuildModeComponent.h"

#include "Building/J3BuildPreview.h"
#include "Building/J3BuilderComponent.h"
#include "Building/J3BuildingBase.h"
#include "Building/J3BuildingSubsystem.h"
#include "Core/J3PlayerState.h"
#include "Data/J3DataSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Player/J3Character.h"


namespace J3BuildModeComponentPrivate
{
	constexpr int32 HotbarSize = 10;
}


UJ3BuildModeComponent::UJ3BuildModeComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	HotbarSlots.SetNum(J3BuildModeComponentPrivate::HotbarSize);
}


bool UJ3BuildModeComponent::TraceAim(FHitResult& OutHit) const
{
	// 시점에서 건설 거리까지. 미리보기와 내 몸은 무시한다.
	const APlayerController* Controller = Cast<APlayerController>(GetOwner());
	FVector ViewLocation;
	FRotator ViewRotation;
	Controller->GetPlayerViewPoint(ViewLocation, ViewRotation);
	const float RangeCm = UJ3DataSubsystem::Get()->GetConfigFloat("build_range_cm", 1500.f);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(J3BuildModeAim), true, Controller->GetPawn());
	Params.AddIgnoredActor(Preview);
	return GetWorld()->LineTraceSingleByChannel(OutHit, ViewLocation, ViewLocation + ViewRotation.Vector() * RangeCm, ECC_Visibility, Params);
}


void UJ3BuildModeComponent::BeginPlacing(FName BuildingId)
{
	// 해방된 건물만 (메뉴·단축 칸 모두)
	const FJ3BuildingDef* Def = UJ3DataSubsystem::Get()->GetBuilding(BuildingId);
	UClass* BuildingClass = Def != nullptr ? Def->BuildingClass.TryLoadClass<AJ3BuildingBase>() : nullptr;
	if (BuildingClass == nullptr)
	{
		return;
	}


	// 철거 모드와는 동시에 쓰지 않는다
	EndDemolish();


	// 미리보기 모형 (이 PC에서만)
	if (Preview == nullptr)
	{
		FActorSpawnParameters Params;
		Params.ObjectFlags |= RF_Transient;
		Preview = GetWorld()->SpawnActor<AJ3BuildPreview>(AJ3BuildPreview::StaticClass(), Params);
	}
	Preview->SetBuildingClass(BuildingClass);
	Preview->SetActorHiddenInGame(false);
	SelectedBuildingId = BuildingId;
	UpdatePreview();
}


void UJ3BuildModeComponent::EndPlacing()
{
	SelectedBuildingId = NAME_None;
	if (Preview != nullptr)
	{
		Preview->SetActorHiddenInGame(true);
	}
}


void UJ3BuildModeComponent::Rotate(int32 Direction)
{
	// 보조 그리드의 회전 스냅과 같은 단위 (Building_Placement, 제안 15°)
	const float StepDeg = UJ3DataSubsystem::Get()->GetConfigFloat("build_rotate_step_deg", 15.f);
	YawDeg = FRotator::NormalizeAxis(YawDeg + (Direction >= 0 ? StepDeg : -StepDeg));
}


void UJ3BuildModeComponent::Confirm()
{
	// 미리보기가 파랑일 때만 요청한다 (서버가 다시 판정한다)
	const AJ3PlayerState* PlayerState = Cast<APlayerController>(GetOwner())->GetPlayerState<AJ3PlayerState>();
	if (!IsPlacing() || !LastCheck.IsOk() || PlayerState == nullptr)
	{
		return;
	}
	PlayerState->GetBuilder()->Server_PlaceBuilding(SelectedBuildingId, LastAimLocation, YawDeg, bGrid);
}


void UJ3BuildModeComponent::PickAimedBuilding()
{
	FHitResult Hit;
	const AJ3BuildingBase* Building = TraceAim(Hit) ? Cast<AJ3BuildingBase>(Hit.GetActor()) : nullptr;
	if (Building != nullptr)
	{
		YawDeg = Building->GetActorRotation().Yaw;
		BeginPlacing(Building->GetBuildingId());
	}
}


void UJ3BuildModeComponent::AssignHotbar(int32 Index, FName BuildingId)
{
	if (HotbarSlots.IsValidIndex(Index))
	{
		HotbarSlots[Index] = BuildingId;
	}
}


void UJ3BuildModeComponent::UseHotbar(int32 Index)
{
	const FName BuildingId = GetHotbar(Index);
	if (!BuildingId.IsNone())
	{
		BeginPlacing(BuildingId);
	}
}


void UJ3BuildModeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (IsPlacing())
	{
		UpdatePreview();
	}
	if (IsDemolishing())
	{
		UpdateDemolish(DeltaTime);
	}
}


void UJ3BuildModeComponent::ToggleDemolish()
{
	if (bDemolishing)
	{
		EndDemolish();
		return;
	}


	// 설치 중이었으면 끝내고 철거 모드로
	EndPlacing();
	if (DemolishHighlight == nullptr)
	{
		FActorSpawnParameters Params;
		Params.ObjectFlags |= RF_Transient;
		DemolishHighlight = GetWorld()->SpawnActor<AJ3BuildPreview>(AJ3BuildPreview::StaticClass(), Params);
		DemolishHighlight->SetActorHiddenInGame(true);
	}
	bDemolishing = true;
	bPrimaryHeld = false;
	DemolishHoldSec = 0.f;
}


void UJ3BuildModeComponent::EndDemolish()
{
	bDemolishing = false;
	bPrimaryHeld = false;
	DemolishHoldSec = 0.f;
	DemolishTarget = nullptr;
	if (DemolishHighlight != nullptr)
	{
		DemolishHighlight->SetActorHiddenInGame(true);
	}
}


void UJ3BuildModeComponent::PressPrimary()
{
	if (IsDemolishing())
	{
		bPrimaryHeld = true;
		DemolishHoldSec = 0.f;
		return;
	}
	Confirm();
}


void UJ3BuildModeComponent::ReleasePrimary()
{
	bPrimaryHeld = false;
	DemolishHoldSec = 0.f;
}


float UJ3BuildModeComponent::GetDemolishProgress() const
{
	const float HoldNeededSec = UJ3DataSubsystem::Get()->GetConfigFloat("demolish_hold_sec", 0.5f);
	if (!IsDemolishing() || !bPrimaryHeld || DemolishTarget == nullptr || !bDemolishTargetInRange)
	{
		return -1.f;
	}
	return HoldNeededSec > 0.f ? FMath::Clamp(DemolishHoldSec / HoldNeededSec, 0.f, 1.f) : 1.f;
}


void UJ3BuildModeComponent::UpdateDemolish(float DeltaTime)
{
	// 조준한 건물 (요청을 보내 사라지는 중인 건물은 빼고)
	FHitResult Hit;
	AJ3BuildingBase* Aimed = TraceAim(Hit) ? Cast<AJ3BuildingBase>(Hit.GetActor()) : nullptr;
	if (Aimed != nullptr && Aimed == RequestedTarget.Get())
	{
		Aimed = nullptr;
	}
	if (Aimed != DemolishTarget.Get())
	{
		// 대상이 바뀌면 처음부터 다시 누른다
		DemolishTarget = Aimed;
		DemolishHoldSec = 0.f;
		if (Aimed != nullptr)
		{
			DemolishHighlight->SetBuildingClass(Aimed->GetClass());
			DemolishHighlight->SetValid(false);
		}
	}


	// 대상 표시. 거리는 서버와 같은 함수로 본다
	const AJ3PlayerState* PlayerState = Cast<APlayerController>(GetOwner())->GetPlayerState<AJ3PlayerState>();
	bDemolishTargetInRange = Aimed != nullptr && PlayerState != nullptr && PlayerState->GetBuilder()->CanDemolish(Aimed);
	DemolishHighlight->SetActorHiddenInGame(Aimed == nullptr);
	if (Aimed == nullptr)
	{
		return;
	}
	const float HighlightScale = UJ3DataSubsystem::Get()->GetConfigFloat("demolish_highlight_scale", 1.02f);
	DemolishHighlight->SetActorTransform(FTransform(Aimed->GetActorRotation(), Aimed->GetActorLocation(), FVector(HighlightScale)));


	// 길게 누르면 요청 (서버가 거리를 다시 확인하고 환급한다)
	if (!bPrimaryHeld || !bDemolishTargetInRange)
	{
		DemolishHoldSec = 0.f;
		return;
	}
	DemolishHoldSec += DeltaTime;
	if (DemolishHoldSec >= UJ3DataSubsystem::Get()->GetConfigFloat("demolish_hold_sec", 0.5f))
	{
		PlayerState->GetBuilder()->Server_Demolish(Aimed);
		RequestedTarget = Aimed;
		DemolishTarget = nullptr;
		DemolishHoldSec = 0.f;
	}
}


void UJ3BuildModeComponent::UpdatePreview()
{
	// 조준점이 없으면(허공) 숨긴다
	FHitResult Hit;
	if (!TraceAim(Hit))
	{
		LastCheck = FJ3PlacementCheck();
		LastCheck.Result = EJ3PlacementResult::NoGround;
		Preview->SetActorHiddenInGame(true);
		return;
	}
	LastAimLocation = Hit.ImpactPoint;


	// 서버와 같은 판정으로 위치와 색을 정한다 (바닥을 못 찾으면 조준점에 그대로 둔다)
	const UJ3BuildingSubsystem* Buildings = GetWorld()->GetSubsystem<UJ3BuildingSubsystem>();
	const AJ3Character* Builder = Cast<AJ3Character>(Cast<APlayerController>(GetOwner())->GetPawn());
	LastCheck = Buildings->CheckPlacement(SelectedBuildingId, LastAimLocation, YawDeg, bGrid, Builder, Preview);
	const bool bHasTransform = LastCheck.Result != EJ3PlacementResult::NoGround && LastCheck.Result != EJ3PlacementResult::UnknownBuilding && !LastCheck.Transform.Equals(FTransform::Identity);
	const FTransform Transform = bHasTransform ? LastCheck.Transform : FTransform(FRotator(0.f, YawDeg, 0.f), LastAimLocation);
	Preview->SetActorTransform(Transform);
	Preview->SetActorHiddenInGame(false);
	Preview->SetValid(LastCheck.IsOk());
}
