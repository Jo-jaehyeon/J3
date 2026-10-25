// Copyright (c) ACCENT. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/LocalPlayer.h"
#include "J3LocalPlayer.generated.h"


// 접속할 때 서버에 보내는 로그인 옵션을 붙인다 (데이터 해시).
UCLASS()
class J3_API UJ3LocalPlayer : public ULocalPlayer
{
	GENERATED_BODY()

public:
	virtual FString GetGameLoginOptions() const override;
};
