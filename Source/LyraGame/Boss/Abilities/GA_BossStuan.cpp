// Fill out your copyright notice in the Description page of Project Settings.


#include "Boss/Abilities/GA_BossStuan.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Boss/Bear/BearBossBase.h"
#include "GameFramework/CharacterMovementComponent.h"

UGA_BossStuan::UGA_BossStuan()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	AbilityTags.AddTag(FGameplayTag::RequestGameplayTag("Boss.Action.Stun"));
	ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag("Boss.State.Stunned"));
}

bool UGA_BossStuan::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                       const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags,
                                       const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}

void UGA_BossStuan::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	UE_LOG(LogTemp, Warning, TEXT("[Stun] GA_BossStuan ActivateAbility 진입"));
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	AActor* RawActor = GetAvatarActorFromActorInfo();
	ABearBossBase* Bear = Cast<ABearBossBase>(RawActor);

	Bear->GetCharacterMovement()->SetMovementMode(MOVE_None);
	UE_LOG(LogTemp, Warning, TEXT("[Stun] 이동 정지 (MOVE_None)"));
	// 몽타주 재생 (GA 종료 시 자동 중단)
	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, StunMontage, 1.0f,
		FName("StunStart"),
		true
	);
	MontageTask->ReadyForActivation();

	// StunDuration 후 스턴 종료
	UAbilityTask_WaitDelay* DelayTask = UAbilityTask_WaitDelay::WaitDelay(this, StunDuration);
	DelayTask->OnFinish.AddDynamic(this, &UGA_BossStuan::StunEnd);
	DelayTask->ReadyForActivation();
}

void UGA_BossStuan::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	AActor* RawActor = GetAvatarActorFromActorInfo();
	ABearBossBase* Bear = Cast<ABearBossBase>(RawActor);
	Bear->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	{
		UCharacterMovementComponent* MoveComp = Bear->GetCharacterMovement();
		UE_LOG(LogTemp, Warning,
			TEXT("[Stun] EndAbility - MOVE_Walking 세팅 후 MovementMode=%d IsFalling=%d IsMovingOnGround=%d"),
			(int32)MoveComp->MovementMode.GetValue(),
			MoveComp->IsFalling(),
			MoveComp->IsMovingOnGround());
	}
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_BossStuan::StunEnd()
{
	UE_LOG(LogTemp, Warning, TEXT("[Stun] StunDuration 종료 → EndAbility"));
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
