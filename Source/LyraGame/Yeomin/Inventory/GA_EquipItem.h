// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/LyraGameplayAbility.h"
#include "GA_EquipItem.generated.h"

/**
 * 
 */
UCLASS()
class LYRAGAME_API UGA_EquipItem : public ULyraGameplayAbility
{
	GENERATED_BODY()
	
	UGA_EquipItem();
	
protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
};
