#include "Boss/Abilities/GA_Boss_NormalAttack.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"

UGA_Boss_NormalAttack::UGA_Boss_NormalAttack()
{
	
	
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor; 
}

void UGA_Boss_NormalAttack::InitializeForCahe(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	CacheHandle = Handle;
	CacheActorInfo = ActorInfo;
	CacheActivationInfo = ActivationInfo;
}

void UGA_Boss_NormalAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                            const FGameplayAbilityActorInfo* ActorInfo,
                                            const FGameplayAbilityActivationInfo ActivationInfo,
                                            const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	InitializeForCahe(Handle, ActorInfo, ActivationInfo);
	
	
	if (AttackMontage == nullptr)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	
	UAbilityTask_PlayMontageAndWait* Task=  UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this,NAME_None,AttackMontage,1.0f);
	
	Task->OnCompleted.AddDynamic(this,&UGA_Boss_NormalAttack::OnMontageCompleted);
	Task->OnCancelled.AddDynamic(this,&UGA_Boss_NormalAttack::OnMontageCancelled);
	Task->OnInterrupted.AddDynamic(this,&UGA_Boss_NormalAttack::OnMontageCancelled);
	

	
	Task->ReadyForActivation();
}

void UGA_Boss_NormalAttack::OnMontageCompleted()
{
	
	// bWasCancelled = false
	EndAbility(CacheHandle,CacheActorInfo,CacheActivationInfo,true,false);
	
}

void UGA_Boss_NormalAttack::OnMontageCancelled()
{

	// bWasCancelled = true
	EndAbility(CacheHandle,CacheActorInfo,CacheActivationInfo,true,true);
}

void UGA_Boss_NormalAttack::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
