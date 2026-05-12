#include "Minion/Abilities/GA_Minion_Death.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "Minion/MinionCharacterBase.h"
#include "GameFramework/CharacterMovementComponent.h"

UGA_Minion_Death::UGA_Minion_Death()
{
	AbilityTags.AddTag(FGameplayTag::RequestGameplayTag("Minion.Action.Death"));
	ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag("Minion.State.Dying"));
}

void UGA_Minion_Death::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	UE_LOG(LogTemp, Warning, TEXT("[Death] GA_Minion_Death ActivateAbility 진입"));

	CacheHandle = Handle;
	CacheActorInfo = ActorInfo;
	CacheActivationInfo = ActivationInfo;

	AMinionCharacterBase* Char = Cast<AMinionCharacterBase>(GetAvatarActorFromActorInfo());
	if (!Char)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (AAIController* AICon = Cast<AAIController>(Char->GetController()))
	{
		if (AICon->BrainComponent)
		{
			AICon->BrainComponent->StopLogic(TEXT("Death"));
		}
	}

	Char->GetCharacterMovement()->DisableMovement();

	if (!DeathMontage)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Death] Minion DeathMontage 미지정 → 즉시 종료"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[Death] Minion DeathMontage 재생 시작: %s"), *GetNameSafe(DeathMontage));
	UAbilityTask_PlayMontageAndWait* Task = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, DeathMontage);
	Task->OnCompleted.AddDynamic(this, &ThisClass::OnDeathMontageCompleted);
	Task->OnInterrupted.AddDynamic(this, &ThisClass::OnDeathMontageCancelled);
	Task->OnCancelled.AddDynamic(this, &ThisClass::OnDeathMontageCancelled);
	Task->ReadyForActivation();
}

void UGA_Minion_Death::OnDeathMontageCompleted()
{
	EndAbility(CacheHandle, CacheActorInfo, CacheActivationInfo, true, false);
}

void UGA_Minion_Death::OnDeathMontageCancelled()
{
	EndAbility(CacheHandle, CacheActorInfo, CacheActivationInfo, true, true);
}
