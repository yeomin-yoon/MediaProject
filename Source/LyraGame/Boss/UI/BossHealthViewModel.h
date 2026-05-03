// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "Boss/BossCharacterBase.h"
#include "BossHealthViewModel.generated.h"

// TODO: Forward declarations 추가 (헤더 include 최소화)
//   - class ABossCharacterBase;
//   - class UAbilitySystemComponent;
//   - struct FOnAttributeChangeData;

/**
 *
 */
UCLASS()
class LYRAGAME_API UBossHealthViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

private :
	UPROPERTY()
	float BossMaxHealth;
	UPROPERTY()
	float BossCurrentHealth;
	public :

	UFUNCTION(BlueprintPure, FieldNotify, Category = "Boss|Status")
	float GetBossMaxHealth()const {return BossMaxHealth;}

	UFUNCTION(BlueprintPure, FieldNotify, Category = "Boss|Status")
	float GetBossHealth()const {return BossCurrentHealth;}

	UFUNCTION(BlueprintPure, FieldNotify, Category = "Player|Status")
	float GetHealthPercent() const
	{
		return BossMaxHealth > 0.f ? BossCurrentHealth / BossMaxHealth : 0.f;
	}

	// Boss 액터를 받아서 ASC 구독 + 초기값 세팅
	UFUNCTION(BlueprintCallable, Category = "Boss|Status")
	void BindToBoss(ABossCharacterBase* Boss);

protected:
	// ============================================================
	// [Setter] 값 변경 + FieldNotify broadcast
	// ============================================================
	// TODO: void SetBossHealth(float NewValue);
	//   - 같은 값이면 early return (불필요한 broadcast 방지)
	//   - BossCurrentHealth = NewValue
	//   - BroadcastFieldValueChanged(GetBossHealth FieldID)
	//   - BroadcastFieldValueChanged(GetHealthPercent FieldID)  ← 파생값도 같이 알려야 ProgressBar 갱신됨
	//
	// TODO: void SetBossMaxHealth(float NewValue);  (동일 패턴)
	void SetBossHealth(float NewValue);
	void SetBossMaxHealth(float NewValue);
	// ============================================================
	// [Callback] ASC가 Attribute 변화 시 호출함
	// ============================================================
	// TODO: void HandleHealthChanged(const FOnAttributeChangeData& Data);
	//   - 본문은 한 줄: SetBossHealth(Data.NewValue);
	//
	// TODO: void HandleMaxHealthChanged(const FOnAttributeChangeData& Data);
	void HandleHealthChanged(const FOnAttributeChangeData& Data);
	void HandleMaxHealthChanged(const FOnAttributeChangeData& Data);
};
