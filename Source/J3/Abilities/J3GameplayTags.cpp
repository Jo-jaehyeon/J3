// Copyright (c) ACCENT. All Rights Reserved.

#include "Abilities/J3GameplayTags.h"


namespace J3Tags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Survival_HealthDelta, "Data.Survival.HealthDelta", "생존 틱의 HP 변화량");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Survival_HungerDelta, "Data.Survival.HungerDelta", "생존 틱의 공복 변화량");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Survival_ThirstDelta, "Data.Survival.ThirstDelta", "생존 틱의 갈증 변화량");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Damage, "Data.Damage", "무기 피해량");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Cooldown, "Data.Cooldown", "무기 쿨다운 시간");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Attack, "InputTag.Attack", "공격 버튼(우클릭)으로 발동하는 능력");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Weapon, "Cooldown.Weapon", "무기 공격 쿨다운 중");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Dead, "State.Dead", "HP 0으로 사망한 상태");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Starving, "State.Starving", "공복 또는 갈증이 0이라 HP가 줄어드는 상태");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Harvesting, "State.Harvesting", "자원 채취 중 (공격·장비 변경·점프·다른 상호작용 불가)");
}
