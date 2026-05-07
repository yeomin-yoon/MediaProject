#include "STTask_Boss_Stun.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeLinker.h"
#include "AIController.h"
#include "Boss/BossCharacterBaseAiController.h"

bool FSTTask_Boss_Stun::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(AIControllerHandle);
	return true;
}

EStateTreeRunStatus FSTTask_Boss_Stun::EnterState(FStateTreeExecutionContext& Context,
                                                   const FStateTreeTransitionResult& Transition) const
{
	AAIController* RawController = Context.GetExternalDataPtr(AIControllerHandle);
	ABossCharacterBaseAiController* BossController = Cast<ABossCharacterBaseAiController>(RawController);
	if (!BossController || !BossController->GetPawn())
	{
		return EStateTreeRunStatus::Failed;
	}

	BossController->StopMovement();

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(BossController->GetPawn());
	if (!ASC)
	{
		return EStateTreeRunStatus::Failed;
	}

	// Stun GA 발동 (Boss.Action.Stun 태그를 가진 GA → GA_BossStuan)
	FGameplayTagContainer StunTags;
	StunTags.AddTag(FGameplayTag::RequestGameplayTag("Boss.Action.Stun"));
	const bool bActivated = ASC->TryActivateAbilitiesByTag(StunTags);
	UE_LOG(LogTemp, Warning, TEXT("[StunTask] EnterState: Stun GA 발동 = %s"),
		bActivated ? TEXT("성공") : TEXT("실패"));

	if (!bActivated)
	{
		return EStateTreeRunStatus::Failed;
	}

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FSTTask_Boss_Stun::Tick(FStateTreeExecutionContext& Context,
                                             const float DeltaTime) const
{
	AAIController* RawController = Context.GetExternalDataPtr(AIControllerHandle);
	if (!RawController || !RawController->GetPawn())
	{
		return EStateTreeRunStatus::Failed;
	}

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(RawController->GetPawn());
	if (!ASC)
	{
		return EStateTreeRunStatus::Failed;
	}

	const FGameplayTag StunTag = FGameplayTag::RequestGameplayTag("Boss.State.Stunned");
	if (!ASC->HasMatchingGameplayTag(StunTag))
	{
		return EStateTreeRunStatus::Succeeded;
	}

	return EStateTreeRunStatus::Running;
}

void FSTTask_Boss_Stun::ExitState(FStateTreeExecutionContext& Context,
                                   const FStateTreeTransitionResult& Transition) const
{
}
