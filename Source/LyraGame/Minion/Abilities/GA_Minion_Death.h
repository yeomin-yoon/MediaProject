#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/LyraGameplayAbility_Death.h"
#include "GA_Minion_Death.generated.h"

class UAnimMontage;

UCLASS()
class LYRAGAME_API UGA_Minion_Death : public ULyraGameplayAbility_Death
{
	GENERATED_BODY()

public:
	UGA_Minion_Death();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

private:
	UFUNCTION()
	void OnDeathMontageCompleted();

	UFUNCTION()
	void OnDeathMontageCancelled();

	UPROPERTY(EditDefaultsOnly, Category = "Minion|Death")
	TObjectPtr<UAnimMontage> DeathMontage;

	FGameplayAbilitySpecHandle CacheHandle;
	const FGameplayAbilityActorInfo* CacheActorInfo = nullptr;
	FGameplayAbilityActivationInfo CacheActivationInfo;
};
