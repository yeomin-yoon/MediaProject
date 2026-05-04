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

	void OnSafetyTimeout();

	UPROPERTY(EditDefaultsOnly, Category = "Boss|AerialAttack")
	float LaunchPower = 1000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|AerialAttack")
	float HitRadius = 500.f;

	// Landed 델리게이트가 어떤 이유로든 안 들어오는 경우(예: 벽 침투/MOVE_None 진입 등) GA 무한 대기 방지용
	UPROPERTY(EditDefaultsOnly, Category = "Boss|AerialAttack")
	float SafetyTimeout = 3.0f;

	FGameplayAbilitySpecHandle CacheHandle;
	const FGameplayAbilityActorInfo* CacheActorInfo = nullptr;
	FGameplayAbilityActivationInfo CacheActivationInfo;

	FTimerHandle SafetyTimerHandle;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameplayEffect> DamageEffectClass;
};
