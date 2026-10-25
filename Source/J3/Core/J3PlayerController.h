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

protected:
	virtual void BeginPlay() override;

	// 기본 키 배치 (Dev_Handoff 6절). 에셋은 BP_J3PlayerController에서 지정한다.
	UPROPERTY(EditDefaultsOnly, Category = "J3|Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;
};
