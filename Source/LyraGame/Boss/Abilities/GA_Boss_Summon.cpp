#include "Boss/Abilities/GA_Boss_Summon.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemComponent.h"

UGA_Boss_Summon::UGA_Boss_Summon()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	AbilityTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Boss.Attack.Summon")));
}

void UGA_Boss_Summon::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!SummonMontage)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, SummonMontage);
	MontageTask->OnCompleted.AddDynamic(this, &UGA_Boss_Summon::OnMontageCompleted);
	MontageTask->OnCancelled.AddDynamic(this, &UGA_Boss_Summon::OnMontageCancelled);
	MontageTask->OnInterrupted.AddDynamic(this, &UGA_Boss_Summon::OnMontageCancelled);
	MontageTask->ReadyForActivation();

	const FGameplayTag SummonEventTag = FGameplayTag::RequestGameplayTag(FName("Boss.EVENT.CREATE"));
	UAbilityTask_WaitGameplayEvent* EventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, SummonEventTag);
	EventTask->EventReceived.AddDynamic(this, &UGA_Boss_Summon::OnSummonEventReceived);
	EventTask->ReadyForActivation();
}

void UGA_Boss_Summon::OnSummonEventReceived(FGameplayEventData Payload)
{
	if (!MinionClass) return;

	AActor* Boss = GetAvatarActorFromActorInfo();
	if (!Boss) return;

	const FVector SpawnLocation = Boss->GetActorLocation()
		+ Boss->GetActorForwardVector() * SpawnOffset.X
		+ Boss->GetActorRightVector() * SpawnOffset.Y
		+ FVector::UpVector * SpawnOffset.Z;

	const FRotator SpawnRotation = Boss->GetActorRotation();

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AActor* Spawned = GetWorld()->SpawnActor<AActor>(MinionClass, SpawnLocation, SpawnRotation, SpawnParams);

	if (Spawned)
	{
		UAbilitySystemComponent* MinionAsc = Spawned->FindComponentByClass<UAbilitySystemComponent>();
		if (MinionAsc)
		{
			FGameplayCueParameters CueParameters;
			CueParameters.Location = SpawnLocation;
			CueParameters.Instigator = GetAvatarActorFromActorInfo();
			MinionAsc->ExecuteGameplayCue(FGameplayTag::RequestGameplayTag(FName("GameplayCue.Boss.Minion.SpawnIn")), CueParameters);
		}
	}
}

void UGA_Boss_Summon::OnMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Boss_Summon::OnMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGA_Boss_Summon::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
