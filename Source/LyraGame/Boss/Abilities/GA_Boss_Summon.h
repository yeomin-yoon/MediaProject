#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/LyraGameplayAbility.h"
#include "GA_Boss_Summon.generated.h"

UCLASS()
class LYRAGAME_API UGA_Boss_Summon : public ULyraGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Boss_Summon();

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
	void OnMontageCompleted();
	UFUNCTION()
	void OnMontageCancelled();


	UFUNCTION()
	void OnSummonEventReceived(FGameplayEventData Payload);


	UPROPERTY(EditDefaultsOnly, Category = "Boss|Summon")
	TObjectPtr<UAnimMontage> SummonMontage;

	
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Summon")
	TSubclassOf<AActor> MinionClass;

	
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Summon")
	FVector SpawnOffset = FVector(300.f, 0.f, 0.f);
};
