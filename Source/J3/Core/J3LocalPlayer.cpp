// Copyright (c) ACCENT. All Rights Reserved.

#include "Core/J3LocalPlayer.h"

#include "Core/J3GameMode.h"
#include "Data/J3DataSubsystem.h"
#include "HAL/IConsoleManager.h"


#if !UE_BUILD_SHIPPING
namespace J3LocalPlayerPrivate
{
	// 한 PC의 PIE에서는 모두 같은 데이터를 쓰므로, 버전 불일치 거부를 시험하려면 해시를 바꿔 보낸다
	TAutoConsoleVariable<FString> CVarFakeDataHash(
		TEXT("j3.Debug.FakeDataHash"),
		TEXT(""),
		TEXT("비어 있지 않으면 접속 시 실제 데이터 해시 대신 이 값을 보낸다 (버전 불일치 테스트용)"));
}
#endif


FString UJ3LocalPlayer::GetGameLoginOptions() const
{
	FString Hash;
	if (const UJ3DataSubsystem* DataSubsystem = UJ3DataSubsystem::Get())
	{
		Hash = DataSubsystem->GetDataHash();
	}


#if !UE_BUILD_SHIPPING
	const FString FakeHash = J3LocalPlayerPrivate::CVarFakeDataHash.GetValueOnGameThread();
	if (!FakeHash.IsEmpty())
	{
		Hash = FakeHash;
	}
#endif


	// 엔진이 이 문자열을 접속 URL에 "?옵션"으로 붙이므로 맨 앞에는 '?'를 넣지 않는다
	FString Options = Super::GetGameLoginOptions();
	if (!Options.IsEmpty())
	{
		Options += TEXT("?");
	}
	return Options + FString::Printf(TEXT("%s=%s"), AJ3GameMode::DataHashOption, *Hash);
}
