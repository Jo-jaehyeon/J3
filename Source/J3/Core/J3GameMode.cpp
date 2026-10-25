// Copyright (c) ACCENT. All Rights Reserved.

#include "Core/J3GameMode.h"

#include "Core/J3PlayerController.h"
#include "Core/J3PlayerState.h"
#include "Data/J3DataSubsystem.h"
#include "Engine/NetDriver.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Player/J3Character.h"

DEFINE_LOG_CATEGORY(LogJ3Net);

const TCHAR* AJ3GameMode::DataHashOption = TEXT("J3DataHash");


AJ3GameMode::AJ3GameMode()
{
	// C++ 기본값. 메시·입력 에셋이 들어간 블루프린트 클래스는 BP_J3GameMode에서 지정한다.
	DefaultPawnClass = AJ3Character::StaticClass();
	PlayerControllerClass = AJ3PlayerController::StaticClass();
	PlayerStateClass = AJ3PlayerState::StaticClass();
}


void AJ3GameMode::BeginPlay()
{
	Super::BeginPlay();


	// Iris가 실제로 켜졌는지 확인용 (단계 2 측정 전에 설정이 먹었는지 바로 보이게)
	const UNetDriver* NetDriver = GetWorld()->GetNetDriver();
	if (NetDriver != nullptr)
	{
		UE_LOG(LogJ3Net, Display, TEXT("리슨 서버 시작: Iris %s"), NetDriver->IsUsingIrisReplication() ? TEXT("사용") : TEXT("미사용(레거시 복제)"));
	}
}


void AJ3GameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
	if (!ErrorMessage.IsEmpty())
	{
		return;
	}
	const UJ3DataSubsystem* DataSubsystem = UJ3DataSubsystem::Get();
	check(DataSubsystem != nullptr);


	// 인원 상한 (호스트 포함)
	const int32 MaxPlayers = DataSubsystem->GetConfigInt("max_players", 4);
	if (GetNumPlayers() >= MaxPlayers)
	{
		ErrorMessage = FString::Printf(TEXT("방이 가득 찼습니다 (최대 %d명)"), MaxPlayers);
		UE_LOG(LogJ3Net, Display, TEXT("참가 거부(인원 상한): %s"), *Address);
		return;
	}


	// 데이터 버전: 아이템·레시피 표가 다르면 시뮬레이션 결과가 갈라지므로 참가를 막는다
	const FString ClientHash = UGameplayStatics::ParseOption(Options, DataHashOption);
	if (ClientHash != DataSubsystem->GetDataHash())
	{
		ErrorMessage = TEXT("버전 불일치: 호스트와 게임 데이터가 다릅니다");
		UE_LOG(LogJ3Net, Display, TEXT("참가 거부(데이터 해시 불일치): %s 클라 '%s' / 서버 '%s'"), *Address, *ClientHash, *DataSubsystem->GetDataHash());
	}
}


void AJ3GameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	UE_LOG(LogJ3Net, Display, TEXT("접속: %s (현재 %d명)"), *GetNameSafe(NewPlayer), GetNumPlayers());
}
