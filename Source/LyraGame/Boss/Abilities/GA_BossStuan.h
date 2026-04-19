// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/LyraGameplayAbility.h"
#include "GA_BossStuan.generated.h"

/**
 * 
 */
UCLASS()
class LYRAGAME_API UGA_BossStuan : public ULyraGameplayAbility
{
	GENERATED_BODY()
public:
	 UGA_BossStuan();
protected:
	
	UPROPERTY(EditDefaultsOnly, Category = "Boss | Combat")
	float StunDuration;
	UPROPERTY(EditDefaultsOnly, Category = "Boss | Combat")
	TObjectPtr<UAnimMontage> StunMontage;
	
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags,
		FGameplayTagContainer* OptionalRelevantTags) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	UFUNCTION()
	void StunEnd();
};
