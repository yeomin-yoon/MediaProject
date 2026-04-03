#include "Boss/Tasks/STTask_Boss_Summon.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeLinker.h"
#include "AIController.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayTagContainer.h"
#include "Boss/BossCharacterBaseAiController.h"

bool FSTTask_Boss_Summon::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(AIControllerHandle);
	return true;
}

EStateTreeRunStatus FSTTask_Boss_Summon::EnterState(FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	Data.bIsEnd = false;
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

	// ASC에 등록된 GA 목록과 태그 출력
	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		UE_LOG(LogTemp, Warning, TEXT("[STTask_Boss_Summon] 등록된 GA: %s | Tags: %s"),
			*GetNameSafe(Spec.Ability),
			*Spec.Ability->AbilityTags.ToStringSimple());
	}

	FGameplayTagContainer ActivationTags;
	ActivationTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Boss.Attack.Summon")));
	const bool bSucceeded = ASC->TryActivateAbilitiesByTag(ActivationTags);
	UE_LOG(LogTemp, Warning, TEXT("[STTask_Boss_Summon] GA 활성화 %s"), bSucceeded ? TEXT("성공") : TEXT("실패"));
	if (!bSucceeded)
	{
		return EStateTreeRunStatus::Failed;
	}

	// GA 종료 시 bIsEnd = true
	const FGameplayTag SummonTag = FGameplayTag::RequestGameplayTag(FName("Boss.Attack.Summon"));
	FInstanceDataType* DataPtr = &Data;
	Data.AbilityEndCallbackHandle = ASC->AbilityEndedCallbacks.AddLambda(
		[DataPtr, SummonTag](UGameplayAbility* EndedAbility)
		{
			if (EndedAbility && EndedAbility->AbilityTags.HasTag(SummonTag))
			{
				UE_LOG(LogTemp, Warning, TEXT("[STTask_Boss_Summon] GA 종료 감지 → Succeeded로 전환"));
				DataPtr->bIsEnd = true;
			}
		}
	);

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FSTTask_Boss_Summon::Tick(FStateTreeExecutionContext& Context,
	const float DeltaTime) const
{
	const FInstanceDataType& Data = Context.GetInstanceData(*this);

	if (Data.bIsEnd)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	return EStateTreeRunStatus::Running;
}

void FSTTask_Boss_Summon::ExitState(FStateTreeExecutionContext& Context,
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
