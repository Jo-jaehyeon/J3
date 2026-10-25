// Copyright (c) ACCENT. All Rights Reserved.

#pragma once

#include "NativeGameplayTags.h"


// 코드에서 쓰는 게임플레이 태그. 태그 이름을 문자열로 흩뿌리지 않도록 여기에 모은다.
namespace J3Tags
{
	// 생존 수치 변화량 (SetByCaller). 수치는 config.csv에서 계산해 넣는다.
	J3_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Survival_HealthDelta);
	J3_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Survival_HungerDelta);
	J3_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Survival_ThirstDelta);

	// 상태
	J3_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Dead);
	J3_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Starving);
}
