// Copyright (c) ACCENT. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "J3GameMode.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogJ3Net, Log, All);


// 리슨 서버 게임 모드. 싱글도 호스트 혼자인 리슨 서버라 같은 클래스를 쓴다.
// 참가 판정(인원 상한, 데이터 버전)은 서버인 여기서만 한다.
UCLASS()
class J3_API AJ3GameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	// 클라이언트가 접속 URL에 붙여 보내는 데이터 해시 옵션 이름
	static const TCHAR* DataHashOption;

	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;

protected:
	virtual void BeginPlay() override;
};
