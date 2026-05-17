#include "CustomStatusAttributeSet.h"
#include "Net/UnrealNetwork.h"

void UCustomStatusAttributeSet::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(
		UCustomStatusAttributeSet,
		AttackPower,
		COND_None,
		REPNOTIFY_Always);

	DOREPLIFETIME_CONDITION_NOTIFY(
		UCustomStatusAttributeSet,
		HealthBonus,
		COND_None,
		REPNOTIFY_Always);

	DOREPLIFETIME_CONDITION_NOTIFY(
		UCustomStatusAttributeSet,
		StaminaBonus,
		COND_None,
		REPNOTIFY_Always);

	DOREPLIFETIME_CONDITION_NOTIFY(
		UCustomStatusAttributeSet,
		DamageReduction,
		COND_None,
		REPNOTIFY_Always);
}

void UCustomStatusAttributeSet::OnRep_AttackPower(
	const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(
		UCustomStatusAttributeSet,
		AttackPower,
		OldValue);
}

void UCustomStatusAttributeSet::OnRep_HealthBonus(
	const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(
		UCustomStatusAttributeSet,
		HealthBonus,
		OldValue);
}

void UCustomStatusAttributeSet::OnRep_StaminaBonus(
	const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(
		UCustomStatusAttributeSet,
		StaminaBonus,
		OldValue);
}

void UCustomStatusAttributeSet::OnRep_DamageReduction(
	const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(
		UCustomStatusAttributeSet,
		DamageReduction,
		OldValue);
}