// Fill out your copyright notice in the Description page of Project Settings.


#include "Boss/UI/BossHealthViewModel.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/LyraHealthSet.h"

// TODO: 추가 include 작성
//   #include "AbilitySystemComponent.h"
//   #include "GameplayEffectExtension.h"             // FOnAttributeChangeData 정의
//   #include "AbilitySystem/Attributes/LyraHealthSet.h"
//   #include "Boss/BossCharacterBase.h"




void UBossHealthViewModel::BindToBoss(ABossCharacterBase* Boss)
{
	if (!Boss) return;
	UAbilitySystemComponent* Asc = Boss->GetAbilitySystemComponent();
	if (!Asc) return;
	const ULyraHealthSet* HealthSet = Asc->GetSet<ULyraHealthSet>();
	if (!HealthSet) return;
	SetBossMaxHealth(HealthSet->GetMaxHealth());
	SetBossHealth(HealthSet->GetHealth());

	// 시작 시점에는 뒤 바도 현재 HP에 맞춰 둠 (0에서 차오르는 연출 방지)
	DelayedHealthPercent = GetHealthPercent();
	DelayHoldTimer = 0.f;
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetDelayedHealthPercent);

	Asc->GetGameplayAttributeValueChangeDelegate(ULyraHealthSet::GetHealthAttribute())
		.AddUObject(this, &UBossHealthViewModel::HandleHealthChanged);

	Asc->GetGameplayAttributeValueChangeDelegate(ULyraHealthSet::GetMaxHealthAttribute())
		.AddUObject(this, &UBossHealthViewModel::HandleMaxHealthChanged);
}
// ============================================================
// SetBossHealth(float NewValue)
// ============================================================
// 1) 같은 값이면 return (FMath::IsNearlyEqual 권장)
// 2) BossCurrentHealth = NewValue;
// 3) BroadcastFieldValueChanged(
//        UE_MVVM_FIELD_NOTIFY_GET_FIELD_ID(UBossHealthViewModel, GetBossHealth)
//    );
// 4) HealthPercent도 갱신 알림:
//    BroadcastFieldValueChanged(
//        UE_MVVM_FIELD_NOTIFY_GET_FIELD_ID(UBossHealthViewModel, GetHealthPercent)
//    );
//
// SetBossMaxHealth: 동일 패턴 (GetBossMaxHealth + GetHealthPercent broadcast)
void UBossHealthViewModel::SetBossHealth(float NewValue)
{
	const float OldValue = BossCurrentHealth;
	if (FMath::IsNearlyEqual(NewValue, OldValue))
	{
		return;
	}
	BossCurrentHealth = NewValue;
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetBossHealth);
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetHealthPercent);
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetCurretHpText);

	if (NewValue < OldValue)
	{
		// 피해를 받았을 때만 hold 타이머 리셋 (연속 피격 시 재시작 효과)
		DelayHoldTimer = DelayHoldDuration;
	}
	else
	{
		// 회복 시 뒤 바도 즉시 위로 맞춤 (천천히 따라가면 어색함)
		DelayHoldTimer = 0.f;
		DelayedHealthPercent = GetHealthPercent();
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetDelayedHealthPercent);
	}
}

void UBossHealthViewModel::TickDelayedHealth(float DeltaTime)
{
	if (DelayHoldTimer > 0.f)
	{
		DelayHoldTimer -= DeltaTime;
		return;
	}

	const float Target = GetHealthPercent();
	if (FMath::IsNearlyEqual(DelayedHealthPercent, Target, KINDA_SMALL_NUMBER))
	{
		return;
	}

	DelayedHealthPercent = FMath::FInterpTo(DelayedHealthPercent, Target, DeltaTime, DelayInterpSpeed);
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetDelayedHealthPercent);
}

void UBossHealthViewModel::SetBossMaxHealth(float NewValue)
{
	if (NewValue == BossMaxHealth)
	{
		return;
	}
	BossMaxHealth = NewValue;
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetBossMaxHealth);
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetHealthPercent);
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetCurretHpText);

}

void UBossHealthViewModel::HandleHealthChanged(const FOnAttributeChangeData& Data)
{
	SetBossHealth(Data.NewValue);
}

void UBossHealthViewModel::HandleMaxHealthChanged(const FOnAttributeChangeData& Data)
{
	SetBossMaxHealth(Data.NewValue);
}





