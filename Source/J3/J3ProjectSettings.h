// Copyright (c) ACCENT. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "J3ProjectSettings.generated.h"

class AJ3BeltVisualizer;
class AJ3PickupItem;
class UMaterialInterface;


// 프로젝트 설정 > J3: 코드가 직접 참조하는 블루프린트(겉모습) 클래스를 지정한다.
// 게임 수치는 여기가 아니라 Data/config.csv에 둔다.
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "J3"))
class J3_API UJ3ProjectSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	// 컨베이어·아이템을 그리는 액터 (메시는 블루프린트에서 지정)
	UPROPERTY(Config, EditAnywhere, Category = "Factory")
	TSoftClassPtr<AJ3BeltVisualizer> BeltVisualizerClass;

	// 바닥에 떨구는 아이템 (모양을 지정한 블루프린트)
	UPROPERTY(Config, EditAnywhere, Category = "Items")
	TSoftClassPtr<AJ3PickupItem> PickupItemClass;

	// 건설 미리보기 재질 (반투명, Color·Opacity 파라미터)
	UPROPERTY(Config, EditAnywhere, Category = "Building")
	TSoftObjectPtr<UMaterialInterface> BuildPreviewMaterial;
};
