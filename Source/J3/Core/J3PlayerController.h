// Copyright (c) ACCENT. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "J3PlayerController.generated.h"

class UInputMappingContext;


// 플레이어 컨트롤러. 키 배치(입력 매핑)는 폰이 바뀌어도 유지되도록 여기서 등록한다.
UCLASS()
class J3_API AJ3PlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	// 개발용 치트 (콘솔: J3Give stone_axe 1). 배포 빌드에서는 동작하지 않는다.
	UFUNCTION(Exec)
	void J3Give(FName ItemId, int32 Count = 1);

	// 개발용 치트 (콘솔: J3Equip 0) 인벤토리 칸 번호의 장비를 장착한다. TODO(단계 1 UI): 인벤토리 UI 클릭으로 대체
	UFUNCTION(Exec)
	void J3Equip(int32 SlotIndex);

protected:
	UFUNCTION(Server, Reliable)
	void Server_DebugGive(FName ItemId, int32 Count);

	virtual void BeginPlay() override;

	// 기본 키 배치 (Dev_Handoff 6절). 에셋은 BP_J3PlayerController에서 지정한다.
	UPROPERTY(EditDefaultsOnly, Category = "J3|Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;
};
