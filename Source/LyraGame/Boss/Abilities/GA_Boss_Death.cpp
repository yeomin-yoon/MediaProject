#include "Boss/Abilities/GA_Boss_Death.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "Boss/BossCharacterBase.h"
#include "GameFramework/CharacterMovementComponent.h"

UGA_Boss_Death::UGA_Boss_Death()
{
	AbilityTags.AddTag(FGameplayTag::RequestGameplayTag("Boss.Action.Death"));
	ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag("Boss.State.Dying"));
}

void UGA_Boss_Death::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	UE_LOG(LogTemp, Warning, TEXT("[Death] GA_Boss_Death ActivateAbility 진입"));

	CacheHandle = Handle;
	CacheActorInfo = ActorInfo;
	CacheActivationInfo = ActivationInfo;

	ABossCharacterBase* Char = Cast<ABossCharacterBase>(GetAvatarActorFromActorInfo());
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
		UE_LOG(LogTemp, Warning, TEXT("[Death] DeathMontage 미지정 → 즉시 종료"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[Death] DeathMontage 재생 시작: %s"), *GetNameSafe(DeathMontage));
	UAbilityTask_PlayMontageAndWait* Task = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, DeathMontage);
	Task->OnCompleted.AddDynamic(this, &ThisClass::OnDeathMontageCompleted);
	Task->OnInterrupted.AddDynamic(this, &ThisClass::OnDeathMontageCancelled);
	Task->OnCancelled.AddDynamic(this, &ThisClass::OnDeathMontageCancelled);
	Task->ReadyForActivation();
}

void UGA_Boss_Death::OnDeathMontageCompleted()
{
	EndAbility(CacheHandle, CacheActorInfo, CacheActivationInfo, true, false);
}

void UGA_Boss_Death::OnDeathMontageCancelled()
{
	EndAbility(CacheHandle, CacheActorInfo, CacheActivationInfo, true, true);
}
