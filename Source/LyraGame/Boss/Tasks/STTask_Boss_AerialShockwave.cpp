#include "Boss/Tasks/STTask_Boss_AerialShockwave.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeLinker.h"
#include "AIController.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayTagContainer.h"
#include "Boss/BossCharacterBaseAiController.h"

bool FSTTask_Boss_AerialShockwave::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(AIControllerHandle);
	return true;
}

EStateTreeRunStatus FSTTask_Boss_AerialShockwave::EnterState(FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	Data.bIsEnd = false;
	Data.LandingElapsed = 0.f;
	Data.AbilityEndCallbackHandle.Reset();

	AAIController* RawController = Context.GetExternalDataPtr(AIControllerHandle);
	ABossCharacterBaseAiController* BossController = Cast<ABossCharacterBaseAiController>(RawController);
	if (!BossController)
	{
		return EStateTreeRunStatus::Failed;
	}

	AActor* BossPawn = BossController->GetPawn();
	if (!BossPawn)
	{
		return EStateTreeRunStatus::Failed;
	}

	BossController->StopMovement();

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(BossPawn);
	if (!ASC)
	{
		return EStateTreeRunStatus::Failed;
	}

	FGameplayTagContainer ActivationTags;
	ActivationTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Boss.Attack.AerialAttack")));
	const bool bSucceeded = ASC->TryActivateAbilitiesByTag(ActivationTags);
	if (!bSucceeded)
	{
		return EStateTreeRunStatus::Running; // Failed 대신 Running으로 스팸 방지
	}

	// GA가 끝났을 때 bIsEnd = true로 설정
	const FGameplayTag AttackTag = FGameplayTag::RequestGameplayTag(FName("Boss.Attack.AerialAttack"));
	FInstanceDataType* DataPtr = &Data;
	Data.AbilityEndCallbackHandle = ASC->AbilityEndedCallbacks.AddLambda(
		[DataPtr, AttackTag](UGameplayAbility* EndedAbility)
		{
			if (EndedAbility && EndedAbility->AbilityTags.HasTag(AttackTag))
			{
				DataPtr->bIsEnd = true;
			}
		}
	);

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FSTTask_Boss_AerialShockwave::Tick(FStateTreeExecutionContext& Context,
	const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);

	if (Data.bIsEnd)
	{
		Data.LandingElapsed += DeltaTime;
		if (Data.LandingElapsed >= LandingStayDuration)
		{
			return EStateTreeRunStatus::Succeeded;
		}
	}

	return EStateTreeRunStatus::Running;
}

void FSTTask_Boss_AerialShockwave::ExitState(FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);

	if (!Data.AbilityEndCallbackHandle.IsValid())
	{
		return;
	}

	AAIController* RawController = Context.GetExternalDataPtr(AIControllerHandle);
	ABossCharacterBaseAiController* BossController = Cast<ABossCharacterBaseAiController>(RawController);
	if (!BossController) { return; }

	AActor* BossPawn = BossController->GetPawn();
	if (!BossPawn) { return; }

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(BossPawn);
	if (!ASC) { return; }
	
	
	ASC->AbilityEndedCallbacks.Remove(Data.AbilityEndCallbackHandle);
	Data.AbilityEndCallbackHandle.Reset();
}
