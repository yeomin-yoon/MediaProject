#include "Boss/Tasks/STTask_Boss_NormalAttack.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeLinker.h"
#include "AIController.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayTagContainer.h"
#include "Boss/BossCharacterBaseAiController.h"

DEFINE_LOG_CATEGORY_STATIC(LogNormalAttack, Log, All);

bool FSTTask_Boss_NormalAttack::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(AIControllerHandle);
	return true;
}

EStateTreeRunStatus FSTTask_Boss_NormalAttack::EnterState(FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
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

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(BossPawn);
	if (!ASC)
	{
		return EStateTreeRunStatus::Failed;
	}

	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		UE_LOG(LogNormalAttack, Log, TEXT("[NormalAttack] 등록 GA: %s | 활성화중=%s | Tags=%s"),
			*GetNameSafe(Spec.Ability),
			Spec.IsActive() ? TEXT("true") : TEXT("false"),
			*Spec.Ability->AbilityTags.ToStringSimple());
	}

	FGameplayTagContainer ActivationTags;
	ActivationTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Boss.Attack.BaseAttack")));
	const bool bSucceeded = ASC->TryActivateAbilitiesByTag(ActivationTags);
	UE_LOG(LogNormalAttack, Warning, TEXT("[NormalAttack] EnterState: GA 활성화 %s"), bSucceeded ? TEXT("성공") : TEXT("실패 ← 멈춤 원인 가능성"));
	if (!bSucceeded)
	{
		return EStateTreeRunStatus::Failed;
	}

	const FGameplayTag AttackTag = FGameplayTag::RequestGameplayTag(FName("Boss.Attack.BaseAttack"));
	FInstanceDataType* DataPtr = &Data;
	Data.AbilityEndCallbackHandle = ASC->AbilityEndedCallbacks.AddLambda(
		[DataPtr, AttackTag](UGameplayAbility* EndedAbility)
		{
			if (EndedAbility && EndedAbility->AbilityTags.HasTag(AttackTag))
			{
				UE_LOG(LogNormalAttack, Log, TEXT("[NormalAttack] GA 종료 감지 → bIsEnd=true"));
				DataPtr->bIsEnd = true;
			}
		}
	);

	UE_LOG(LogNormalAttack, Log, TEXT("[NormalAttack] EnterState: 콜백 등록 완료 → Running"));
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
	if (!BossController)
	{
		return;
	}

	AActor* BossPawn = BossController->GetPawn();
	if (!BossPawn)
	{
		return;
	}

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(BossPawn);
	if (!ASC)
	{
		return;
	}

	ASC->AbilityEndedCallbacks.Remove(Data.AbilityEndCallbackHandle);
	Data.AbilityEndCallbackHandle.Reset();
}
