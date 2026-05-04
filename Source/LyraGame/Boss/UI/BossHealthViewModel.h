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

	// 뒤에 깔리는 "딜레이 바"가 따라갈 퍼센트 (0~1)
	UPROPERTY()
	float DelayedHealthPercent = 1.f;

	// 피해 받은 직후 잠깐 멈춰있는 시간 (초)
	UPROPERTY()
	float DelayHoldTimer = 0.f;

	// 튜닝값
	static constexpr float DelayHoldDuration = 0.4f;
	static constexpr float DelayInterpSpeed  = 1.5f;

	public :

	UFUNCTION(BlueprintPure, FieldNotify, Category = "Boss|Status")
	float GetBossMaxHealth()const {return BossMaxHealth;}

	UFUNCTION(BlueprintPure, FieldNotify, Category = "Boss|Status")
	float GetBossHealth()const {return BossCurrentHealth;}
	
	UFUNCTION(BlueprintPure, FieldNotify, Category = "Boss|Status")
	FText GetCurretHpText() const
	{
		return FText::FromString(FString::Printf(TEXT("%d / %d"),
			FMath::FloorToInt(BossCurrentHealth),
			FMath::FloorToInt(BossMaxHealth)));
	}

	UFUNCTION(BlueprintPure, FieldNotify, Category = "Player|Status")
	float GetHealthPercent() const
	{
		return BossMaxHealth > 0.f ? BossCurrentHealth / BossMaxHealth : 0.f;
	}
	
	
	UFUNCTION(BlueprintPure, FieldNotify, Category = "Boss|Status")
	float GetDelayedHealthPercent() const { return DelayedHealthPercent; }

	
	UFUNCTION(BlueprintCallable, Category = "Boss|Status")
	void TickDelayedHealth(float DeltaTime);

	
	UFUNCTION(BlueprintCallable, Category = "Boss|Status")
	void BindToBoss(ABossCharacterBase* Boss);

protected:
	
	
	void SetBossHealth(float NewValue);
	void SetBossMaxHealth(float NewValue);
	
	void HandleHealthChanged(const FOnAttributeChangeData& Data);
	void HandleMaxHealthChanged(const FOnAttributeChangeData& Data);
};
