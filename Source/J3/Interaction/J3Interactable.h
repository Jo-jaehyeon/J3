// Copyright (c) ACCENT. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "J3Interactable.generated.h"

class AJ3Character;


UINTERFACE(MinimalAPI, NotBlueprintable)
class UJ3Interactable : public UInterface
{
	GENERATED_BODY()
};


// G키로 상호작용할 수 있는 대상 (떨어진 아이템, 자원 노드, 건물, AI 모듈 등).
// 대상은 조준점이 가리키는 것으로 정하고, 거리·조건 확인과 실행은 서버가 한다 (Player_And_Survival 1.2).
class J3_API IJ3Interactable
{
	GENERATED_BODY()

public:
	// 지금 이 플레이어가 상호작용할 수 있는지 (서버·클라이언트 모두에서 호출)
	virtual bool CanInteract(const AJ3Character* Interactor) const = 0;

	// 서버 전용: 상호작용 실행
	virtual void Interact(AJ3Character* Interactor) = 0;
};
