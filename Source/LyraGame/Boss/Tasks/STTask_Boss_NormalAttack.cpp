#include "Boss/Tasks/STTask_Boss_NormalAttack.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeLinker.h"
#include "AIController.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayTagContainer.h"
#include "Boss/BossCharacterBaseAiController.h"

bool FSTTask_Boss_NormalAttack::Link(FStateTreeLinker& Linker)
{
	// TODO: AIControllerHandle 등록 (MoveToRange 참고)
	Linker.LinkExternalData(AIControllerHandle);
	return true;
}

EStateTreeRunStatus FSTTask_Boss_NormalAttack::EnterState(FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	UE_LOG(LogTemp,Warning,TEXT("공격"));
	
	Data.AbilityEndCallbackHandle.Reset();
	Data.bIsEnd = false;
	
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

	UAbilitySystemComponent* AbilitySystemComponent = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(BossPawn);
	if (!AbilitySystemComponent)
	{
		return EStateTreeRunStatus::Failed;
	}

	
	FGameplayTagContainer ActivationTags;
	ActivationTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Boss.Attack.BaseAttack")));
	const bool isSucced = AbilitySystemComponent->TryActivateAbilitiesByTag(ActivationTags);
	UE_LOG(LogTemp, Warning, TEXT("NormalAttack Activate: %s"), isSucced ? TEXT("SUCCESS") : TEXT("FAILED"));
	if (!isSucced)
	{
		return EStateTreeRunStatus::Failed;
	}

	
	const FGameplayTag AttackTag = FGameplayTag::RequestGameplayTag(FName("Boss.Attack.BaseAttack"));
	FInstanceDataType* DataPtr = &Data;
	Data.AbilityEndCallbackHandle = AbilitySystemComponent->AbilityEndedCallbacks.AddLambda(
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

EStateTreeRunStatus FSTTask_Boss_NormalAttack::Tick(FStateTreeExecutionContext& Context,
	const float DeltaTime) const
{

	FInstanceDataType& Data = Context.GetInstanceData(*this);
	
	if (Data.bIsEnd)
	{
		return EStateTreeRunStatus::Succeeded;
	}
	
	return EStateTreeRunStatus::Running;
}

void FSTTask_Boss_NormalAttack::ExitState(FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	
	if (!Data.AbilityEndCallbackHandle.IsValid())
	{
		return;
	}

	
	AAIController* RawController = Context.GetExternalDataPtr(AIControllerHandle);
	ABossCharacterBaseAiController* BossController = Cast<ABossCharacterBaseAiController>(RawController);
	AActor* BossPawn = BossController->GetPawn();
	if (!BossPawn)
	{
		return;
	}
	UAbilitySystemComponent* AbilitySystemComponent = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(BossPawn);
	
	AbilitySystemComponent->AbilityEndedCallbacks.Remove(Data.AbilityEndCallbackHandle);
	
	Data.AbilityEndCallbackHandle.Reset();
}
