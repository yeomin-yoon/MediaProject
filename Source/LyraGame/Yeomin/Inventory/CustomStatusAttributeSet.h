// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/LyraAttributeSet.h"
#include "CustomStatusAttributeSet.generated.h"

/**
 * 
 */

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)


UCLASS()
class LYRAGAME_API UCustomStatusAttributeSet : public ULyraAttributeSet
{
	GENERATED_BODY()
	
public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 공격력
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_AttackPower, Category="Stat")
	FGameplayAttributeData AttackPower;
	ATTRIBUTE_ACCESSORS(UCustomStatusAttributeSet, AttackPower)

	// 받는 데미지 감소
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_DamageReduction, Category="Stat")
	FGameplayAttributeData DamageReduction;
	ATTRIBUTE_ACCESSORS(UCustomStatusAttributeSet, DamageReduction)

protected:
	UFUNCTION()
	void OnRep_AttackPower(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_DamageReduction(const FGameplayAttributeData& OldValue);
};
