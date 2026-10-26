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

	// 전투 수치 (SetByCaller). 수치는 equipment.csv
	J3_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Damage);
	J3_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Cooldown);

	// 입력: 무기 종류와 상관없이 같은 공격 버튼(우클릭)
	J3_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Attack);

	// 쿨다운
	J3_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Weapon);

	// 상태
	J3_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Dead);
	J3_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Starving);
}
