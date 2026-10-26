// Copyright (c) ACCENT. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "J3PlayerController.generated.h"

class UInputMappingContext;
class UJ3CraftingWidget;
class UJ3HUDWidget;
class UJ3InventoryWidget;


// 플레이어 컨트롤러. 키 배치(입력 매핑)는 폰이 바뀌어도 유지되도록 여기서 등록한다.
UCLASS()
class J3_API AJ3PlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	// 개발용 치트 (콘솔: J3Give stone_axe 1). 배포 빌드에서는 동작하지 않는다.
	UFUNCTION(Exec)
	void J3Give(FName ItemId, int32 Count = 1);

	// 개발용 치트 (콘솔: J3Build foundation 0) 조준한 곳에 건물을 짓는다. TODO(단계 4 건설 모드): 건설 메뉴로 대체
	UFUNCTION(Exec)
	void J3Build(FName BuildingId, float YawDeg = 0.f);

	// 개발용 치트 (콘솔: J3Demolish) 조준한 건물을 철거한다. TODO(단계 4 철거 모드): F키로 대체
	UFUNCTION(Exec)
	void J3Demolish();

	// I키: 인벤토리 화면 열기/닫기. 열려 있는 동안 마우스 커서가 나오고 시점 회전은 멈춘다.
	void ToggleInventoryScreen();

	// 제작 장소에 말을 걸면 서버가 이 PC에 화면을 열라고 알린다
	UFUNCTION(Client, Reliable)
	void Client_OpenCraftingScreen(AActor* Station);

	void CloseCraftingScreen();

protected:
	UFUNCTION(Server, Reliable)
	void Server_DebugGive(FName ItemId, int32 Count);

	virtual void BeginPlay() override;

	// 채취 중에는 시점을 시작 방향 기준 일정 각도 안으로 묶는다 (채취는 집중하는 행동)
	virtual void UpdateRotation(float DeltaTime) override;

	// 기본 키 배치 (Dev_Handoff 6절). 에셋은 BP_J3PlayerController에서 지정한다.
	UPROPERTY(EditDefaultsOnly, Category = "J3|Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	// UI 위젯 클래스 (블루프린트에서 겉모습을 바꾼 하위 클래스로 교체 가능)
	UPROPERTY(EditDefaultsOnly, Category = "J3|UI")
	TSubclassOf<UJ3HUDWidget> HUDWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "J3|UI")
	TSubclassOf<UJ3InventoryWidget> InventoryWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "J3|UI")
	TSubclassOf<UJ3CraftingWidget> CraftingWidgetClass;

private:
	UPROPERTY()
	TObjectPtr<UJ3HUDWidget> HUDWidget;

	UPROPERTY()
	TObjectPtr<UJ3InventoryWidget> InventoryWidget;

	UPROPERTY()
	TObjectPtr<UJ3CraftingWidget> CraftingWidget;

	// 화면(인벤토리·제작)이 하나라도 열려 있으면 커서를 보이고 시점 회전을 멈춘다
	void RefreshUIInputMode(UUserWidget* FocusWidget);

	// 조준점이 가리키는 곳 (건설 거리까지)
	bool TraceFromView(FHitResult& OutHit) const;

	// 채취를 시작한 순간의 시점 방향
	bool bWasHarvesting = false;
	FRotator HarvestLookCenter;
};
