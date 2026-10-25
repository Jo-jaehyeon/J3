// Copyright (c) ACCENT. All Rights Reserved.

#include "Abilities/J3GameplayTags.h"


namespace J3Tags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Survival_HealthDelta, "Data.Survival.HealthDelta", "생존 틱의 HP 변화량");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Survival_HungerDelta, "Data.Survival.HungerDelta", "생존 틱의 공복 변화량");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Survival_ThirstDelta, "Data.Survival.ThirstDelta", "생존 틱의 갈증 변화량");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Dead, "State.Dead", "HP 0으로 사망한 상태");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Starving, "State.Starving", "공복 또는 갈증이 0이라 HP가 줄어드는 상태");
}
