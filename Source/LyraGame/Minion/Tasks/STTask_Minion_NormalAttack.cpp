#include "Minion/Tasks/STTask_Minion_NormalAttack.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeLinker.h"
#include "AIController.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayTagContainer.h"

bool FSTTask_Minion_NormalAttack::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(AIControllerHandle);
	return true;
}

EStateTreeRunStatus FSTTask_Minion_NormalAttack::EnterState(FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	Data.bIsEnd = false;
	Data.AbilityEndCallbackHandle.Reset();

	UE_LOG(LogTemp, Warning, TEXT("[Minion] NormalAttack EnterState 진입"));

	AAIController* Controller = Context.GetExternalDataPtr(AIControllerHandle);
	if (!Controller || !Controller->GetPawn())
	{
		UE_LOG(LogTemp, Error, TEXT("[Minion] NormalAttack - Controller 또는 Pawn 없음"));
		return EStateTreeRunStatus::Failed;
	}

	Controller->StopMovement();

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Controller->GetPawn());
	if (!ASC)
	{
		UE_LOG(LogTemp, Error, TEXT("[Minion] NormalAttack - ASC 없음"));
		return EStateTreeRunStatus::Failed;
	}

	FGameplayTagContainer ActivationTags;
	ActivationTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Minion.Attack.BaseAttack")));
	const bool bSucceeded = ASC->TryActivateAbilitiesByTag(ActivationTags);
	UE_LOG(LogTemp, Warning, TEXT("[Minion] NormalAttack Activate: %s"), bSucceeded ? TEXT("SUCCESS") : TEXT("FAILED"));
	if (!bSucceeded)
	{
		return EStateTreeRunStatus::Failed;
	}

	const FGameplayTag AttackTag = FGameplayTag::RequestGameplayTag(FName("Minion.Attack.BaseAttack"));
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

EStateTreeRunStatus FSTTask_Minion_NormalAttack::Tick(FStateTreeExecutionContext& Context,
	const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	return Data.bIsEnd ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Running;
}

void FSTTask_Minion_NormalAttack::ExitState(FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	if (!Data.AbilityEndCallbackHandle.IsValid())
	{
		return;
	}

	AAIController* Controller = Context.GetExternalDataPtr(AIControllerHandle);
	if (!Controller || !Controller->GetPawn())
	{
		return;
	}

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Controller->GetPawn());
	if (ASC)
	{
		ASC->AbilityEndedCallbacks.Remove(Data.AbilityEndCallbackHandle);
	}

	Data.AbilityEndCallbackHandle.Reset();
}
