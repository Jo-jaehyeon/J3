// Copyright (c) ACCENT. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/J3Interactable.h"
#include "J3CraftingStation.generated.h"

class UJ3CraftingStationComponent;
class UStaticMeshComponent;


// 맵에 놓는 제작 장소 (비행기 속 AI 모듈 단말, Tutorial 2단계). G키로 말을 걸면 제작 화면이 열린다.
// 모양은 블루프린트(BP_J3AIModule)에서 지정한다.
UCLASS()
class J3_API AJ3CraftingStation : public AActor, public IJ3Interactable
{
	GENERATED_BODY()

public:
	AJ3CraftingStation();

	virtual bool CanInteract(const AJ3Character* Interactor) const override;
	virtual void Interact(AJ3Character* Interactor) override;

protected:
	UPROPERTY(VisibleAnywhere, Category = "J3|Crafting")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, Category = "J3|Crafting")
	TObjectPtr<UJ3CraftingStationComponent> Station;
};
