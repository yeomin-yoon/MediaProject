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
	if (NewValue == BossCurrentHealth)
	{
		return;
	}
	BossCurrentHealth = NewValue;
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetBossHealth);
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetHealthPercent);


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
	
}

void UBossHealthViewModel::HandleHealthChanged(const FOnAttributeChangeData& Data)
{
	SetBossHealth(Data.NewValue);
}

void UBossHealthViewModel::HandleMaxHealthChanged(const FOnAttributeChangeData& Data)
{
	SetBossMaxHealth(Data.NewValue);
}





