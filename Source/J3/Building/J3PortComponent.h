// Copyright (c) ACCENT. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "J3PortComponent.generated.h"


UENUM(BlueprintType)
enum class EJ3PortKind : uint8
{
	// 컨베이어로 아이템이 오가는 곳
	Item,
	// 전기줄 연결점
	Power,
};


UENUM(BlueprintType)
enum class EJ3PortDirection : uint8
{
	In,
	Out,
};


// 건물의 입출력 포트 (Building_Placement 2.1). 건물 블루프린트에 붙여 위치를 정하고, X축이 바깥을 향하게 놓는다.
// 설치할 때 근처 건물의 맞는 포트(같은 종류, 반대 방향)와 마주 보도록 자동으로 붙는다.
// TODO(아트): 메시 소켓(Art_Direction)으로 포트를 정하게 되면 소켓에서 이 정보를 만든다
UCLASS(ClassGroup = (J3), meta = (BlueprintSpawnableComponent))
class J3_API UJ3PortComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "J3|Port")
	EJ3PortKind Kind = EJ3PortKind::Item;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "J3|Port")
	EJ3PortDirection Direction = EJ3PortDirection::In;

	// 두 포트가 서로 이어질 수 있는지 (같은 종류, 입력↔출력)
	bool CanConnectTo(EJ3PortKind OtherKind, EJ3PortDirection OtherDirection) const
	{
		return Kind == OtherKind && Direction != OtherDirection;
	}
};


// 건물 클래스에 정의된 포트 하나 (건물 원점 기준)
struct FJ3PortInfo
{
	EJ3PortKind Kind = EJ3PortKind::Item;
	EJ3PortDirection Direction = EJ3PortDirection::In;
	FTransform Transform;
};
