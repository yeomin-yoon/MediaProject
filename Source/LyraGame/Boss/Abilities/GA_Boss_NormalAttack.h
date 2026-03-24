#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/LyraGameplayAbility.h"
#include "GA_Boss_NormalAttack.generated.h"

class UAnimMontage;
class UAbilityTask_PlayMontageAndWait;

UCLASS()
class LYRAGAME_API UGA_Boss_NormalAttack : public ULyraGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Boss_NormalAttack();
	void InitializeForCahe(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	                       FGameplayAbilityActivationInfo ActivationInfo);

protected:
	FGameplayAbilitySpecHandle CacheHandle;
	const FGameplayAbilityActorInfo* CacheActorInfo;
	FGameplayAbilityActivationInfo CacheActivationInfo;

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
	// 몽타주 재생 완료 시 호출되는 콜백
	UFUNCTION()
	void OnMontageCompleted();

	// 몽타주가 중간에 취소됐을 때 호출되는 콜백
	UFUNCTION()
	void OnMontageCancelled();

	// 에디터에서 몽타주 에셋 지정
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Attack")
	TObjectPtr<UAnimMontage> AttackMontage;
};
