#include "Minion/Abilities/GA_Minion_NormalAttack.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"

UGA_Minion_NormalAttack::UGA_Minion_NormalAttack()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UGA_Minion_NormalAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	CacheHandle = Handle;
	CacheActorInfo = ActorInfo;
	CacheActivationInfo = ActivationInfo;

	if (!AttackMontage)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilityTask_PlayMontageAndWait* Task = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, AttackMontage, 1.0f);

	Task->OnCompleted.AddDynamic(this, &UGA_Minion_NormalAttack::OnMontageCompleted);
	Task->OnCancelled.AddDynamic(this, &UGA_Minion_NormalAttack::OnMontageCancelled);
	Task->OnInterrupted.AddDynamic(this, &UGA_Minion_NormalAttack::OnMontageCancelled);

	Task->ReadyForActivation();
}

void UGA_Minion_NormalAttack::OnMontageCompleted()
{
	EndAbility(CacheHandle, CacheActorInfo, CacheActivationInfo, true, false);
}

void UGA_Minion_NormalAttack::OnMontageCancelled()
{
	EndAbility(CacheHandle, CacheActorInfo, CacheActivationInfo, true, true);
}

void UGA_Minion_NormalAttack::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
