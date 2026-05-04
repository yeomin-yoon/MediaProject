#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/LyraGameplayAbility.h"
#include "GA_Boss_Charge.generated.h"

UCLASS()
class LYRAGAME_API UGA_Boss_Charge : public ULyraGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Boss_Charge();
	

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
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags,
		FGameplayTagContainer* OptionalRelevantTags) const override;
	FGameplayAbilitySpecHandle CacheHandle;
	const FGameplayAbilityActorInfo* CacheActorInfo;
	FGameplayAbilityActivationInfo CacheActivationInfo;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Combat")
	float ChargeSpd=1300.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Combat")
	float ChargeSecond=6.0f;
	FTimerHandle TimerHandle;

private:
	// TODO: OnActorHit 콜백 함수 선언
	// 규칙: UFUNCTION() 반드시 붙여야 함, 파라미터 4개 고정
	// UFUNCTION()
	// void OnChargeHit(AActor* SelfActor, AActor* OtherActor, FVector NormalImpulse, const FHitResult& Hit);
	UFUNCTION()
	void OnChargeHit(AActor* SelfActor, AActor* OtherActor, FVector NormalImpulse, const FHitResult& Hit);
	
};
