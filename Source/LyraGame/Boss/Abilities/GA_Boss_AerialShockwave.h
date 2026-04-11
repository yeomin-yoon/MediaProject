#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/LyraGameplayAbility.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "GA_Boss_AerialShockwave.generated.h"

class ABearBossBase;

UCLASS()
class LYRAGAME_API UGA_Boss_AerialShockwave : public ULyraGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Boss_AerialShockwave();

public:
	UPROPERTY(EditDefaultsOnly, Category="FX")
	TObjectPtr<UNiagaraSystem> JumpStartFX;

	UPROPERTY(EditDefaultsOnly, Category="FX")
	TObjectPtr<UNiagaraSystem> JumpEndFX;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

private:
	UFUNCTION()
	void OnBossLanded();

	UPROPERTY(EditDefaultsOnly, Category = "Boss|AerialAttack")
	float LaunchPower = 1000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|AerialAttack")
	float HitRadius = 500.f;

	FGameplayAbilitySpecHandle CacheHandle;
	const FGameplayAbilityActorInfo* CacheActorInfo = nullptr;
	FGameplayAbilityActivationInfo CacheActivationInfo;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameplayEffect> DamageEffectClass;
};
