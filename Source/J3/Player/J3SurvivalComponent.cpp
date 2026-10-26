// Copyright (c) ACCENT. All Rights Reserved.

#include "Player/J3SurvivalComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Abilities/J3GameplayTags.h"
#include "Abilities/J3SurvivalAttributeSet.h"
#include "Abilities/J3SurvivalEffects.h"
#include "Data/J3DataSubsystem.h"
#include "HAL/IConsoleManager.h"
#include "TimerManager.h"


namespace J3SurvivalComponentPrivate
{
#if !UE_BUILD_SHIPPING
	// 25분·40분짜리 감소를 테스트에서 빨리 보기 위한 배속
	TAutoConsoleVariable<float> CVarSurvivalTimeScale(
		TEXT("j3.Debug.SurvivalTimeScale"),
		1.f,
		TEXT("생존 수치 감소·회복 배속 (테스트용, 1 = 정상)"));
#endif


	float GetTimeScale()
	{
#if !UE_BUILD_SHIPPING
		return FMath::Max(0.f, CVarSurvivalTimeScale.GetValueOnGameThread());
#else
		return 1.f;
#endif
	}


	// 최대치를 drain_sec 동안 0으로 만드는 속도로 Seconds만큼 줄어든 양 (음수)
	float DrainAmount(float MaxValue, float DrainSec, float Seconds)
	{
		return DrainSec > 0.f ? -MaxValue / DrainSec * Seconds : 0.f;
	}
}


UJ3SurvivalComponent::UJ3SurvivalComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}


void UJ3SurvivalComponent::BeginPlay()
{
	Super::BeginPlay();

	// 생존 계산은 서버만 한다. 클라이언트는 복제된 속성만 본다.
	if (!GetOwner()->HasAuthority())
	{
		return;
	}
	const UJ3DataSubsystem* Data = UJ3DataSubsystem::Get();
	TickIntervalSec = FMath::Max(0.1f, Data->GetConfigFloat("survival_tick_sec", TickIntervalSec));
	GetWorld()->GetTimerManager().SetTimer(TickTimer, this, &UJ3SurvivalComponent::TickSurvival, TickIntervalSec, true);
}


void UJ3SurvivalComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorld()->GetTimerManager().ClearTimer(TickTimer);
	Super::EndPlay(EndPlayReason);
}


UAbilitySystemComponent* UJ3SurvivalComponent::GetAbilitySystem() const
{
	const IAbilitySystemInterface* Owner = Cast<IAbilitySystemInterface>(GetOwner());
	return Owner != nullptr ? Owner->GetAbilitySystemComponent() : nullptr;
}


void UJ3SurvivalComponent::InitializeAttributes(UAbilitySystemComponent* AbilitySystem)
{
	check(GetOwner()->HasAuthority());

	// 최대치 → 현재 값을 가득 채운다 (새 프로필은 공복·갈증 가득, Multiplayer_Architecture 4.3)
	const UJ3DataSubsystem* Data = UJ3DataSubsystem::Get();
	const float MaxHealth = Data->GetConfigFloat("hp_max", 100.f);
	const float MaxHunger = Data->GetConfigFloat("hunger_max", 100.f);
	const float MaxThirst = Data->GetConfigFloat("thirst_max", 100.f);
	AbilitySystem->SetNumericAttributeBase(UJ3SurvivalAttributeSet::GetMaxHealthAttribute(), MaxHealth);
	AbilitySystem->SetNumericAttributeBase(UJ3SurvivalAttributeSet::GetMaxHungerAttribute(), MaxHunger);
	AbilitySystem->SetNumericAttributeBase(UJ3SurvivalAttributeSet::GetMaxThirstAttribute(), MaxThirst);
	AbilitySystem->SetNumericAttributeBase(UJ3SurvivalAttributeSet::GetHealthAttribute(), MaxHealth);
	AbilitySystem->SetNumericAttributeBase(UJ3SurvivalAttributeSet::GetHungerAttribute(), MaxHunger);
	AbilitySystem->SetNumericAttributeBase(UJ3SurvivalAttributeSet::GetThirstAttribute(), MaxThirst);
	UpdateStateTags(AbilitySystem);
}


void UJ3SurvivalComponent::TickSurvival()
{
	using namespace J3SurvivalComponentPrivate;

	// 사망 중에는 멈춘다 (부활 처리는 단계 6 기지에서)
	UAbilitySystemComponent* AbilitySystem = GetAbilitySystem();
	const UJ3SurvivalAttributeSet* Attributes = AbilitySystem != nullptr ? AbilitySystem->GetSet<UJ3SurvivalAttributeSet>() : nullptr;
	if (Attributes == nullptr || AbilitySystem->HasMatchingGameplayTag(J3Tags::State_Dead))
	{
		return;
	}


	// 공복·갈증 감소량
	const UJ3DataSubsystem* Data = UJ3DataSubsystem::Get();
	const float Seconds = TickIntervalSec * GetTimeScale();
	const float HungerDelta = DrainAmount(Attributes->GetMaxHunger(), Data->GetConfigFloat("hunger_drain_sec", 0.f), Seconds);
	const float ThirstDelta = DrainAmount(Attributes->GetMaxThirst(), Data->GetConfigFloat("thirst_drain_sec", 0.f), Seconds);
	const float NextHunger = Attributes->GetHunger() + HungerDelta;
	const float NextThirst = Attributes->GetThirst() + ThirstDelta;


	// HP 감소: 둘 중 하나라도 0이 된 뒤 유예 시간이 지나면 감소, 둘 다 0이면 배수 적용.
	// 유예가 틱 중간에 끝나면 넘은 시간만큼만 깎아 틱 간격과 상관없이 같은 결과가 나오게 한다.
	float HealthDelta = 0.f;
	const bool bHungerEmpty = NextHunger <= 0.f;
	const bool bThirstEmpty = NextThirst <= 0.f;
	if (bHungerEmpty || bThirstEmpty)
	{
		const float GraceSec = Data->GetConfigFloat("starve_grace_sec", 0.f);
		const float PrevElapsed = StarvingElapsedSec;
		StarvingElapsedSec += Seconds;
		const float LossSec = FMath::Clamp(StarvingElapsedSec - FMath::Max(GraceSec, PrevElapsed), 0.f, Seconds);
		const float Multiplier = bHungerEmpty && bThirstEmpty ? Data->GetConfigFloat("starve_both_multiplier", 1.f) : 1.f;
		HealthDelta = -Data->GetConfigFloat("starve_hp_loss_per_sec", 1.f) * Multiplier * LossSec;
	}
	else
	{
		StarvingElapsedSec = 0.f;
	}


	// 자연 회복: 둘 다 기준 이상일 때
	// TODO(기획-미결정): 회복 기준(regen_threshold_ratio)은 config 임시값
	const float RegenRatio = Data->GetConfigFloat("regen_threshold_ratio", 0.5f);
	if (NextHunger >= Attributes->GetMaxHunger() * RegenRatio && NextThirst >= Attributes->GetMaxThirst() * RegenRatio)
	{
		HealthDelta = Data->GetConfigFloat("regen_hp_per_sec", 0.f) * Seconds;
	}


	// GAS 효과로 반영
	FGameplayEffectSpecHandle Spec = AbilitySystem->MakeOutgoingSpec(UJ3GE_SurvivalTick::StaticClass(), 1.f, AbilitySystem->MakeEffectContext());
	Spec.Data->SetSetByCallerMagnitude(J3Tags::Data_Survival_HealthDelta, HealthDelta);
	Spec.Data->SetSetByCallerMagnitude(J3Tags::Data_Survival_HungerDelta, HungerDelta);
	Spec.Data->SetSetByCallerMagnitude(J3Tags::Data_Survival_ThirstDelta, ThirstDelta);
	AbilitySystem->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());


	UpdateStateTags(AbilitySystem);
}


void UJ3SurvivalComponent::UpdateStateTags(UAbilitySystemComponent* AbilitySystem) const
{
	// 상태 태그는 다른 클라이언트도 알아야 하므로(쓰러짐 표시 등) 복제 태그로 둔다
	const UJ3SurvivalAttributeSet* Attributes = AbilitySystem->GetSet<UJ3SurvivalAttributeSet>();
	const bool bStarving = Attributes->GetHunger() <= 0.f || Attributes->GetThirst() <= 0.f;
	const bool bDead = Attributes->GetHealth() <= 0.f;
	AbilitySystem->SetLooseGameplayTagCount(J3Tags::State_Starving, bStarving ? 1 : 0, EGameplayTagReplicationState::TagOnly);
	AbilitySystem->SetLooseGameplayTagCount(J3Tags::State_Dead, bDead ? 1 : 0, EGameplayTagReplicationState::TagOnly);
}
