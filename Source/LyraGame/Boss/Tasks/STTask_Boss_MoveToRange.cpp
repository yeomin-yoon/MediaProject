#include "STTask_Boss_MoveToRange.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeLinker.h"
#include "AIController.h"
#include "Boss/BossCharacterBaseAiController.h"


bool FSTTask_Boss_MoveToRange::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(AIControllerHandle);
	return true;
}

EStateTreeRunStatus FSTTask_Boss_MoveToRange::EnterState(FStateTreeExecutionContext& Context,
                                                          const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);

	AAIController* RawController = Context.GetExternalDataPtr(AIControllerHandle);
	ABossCharacterBaseAiController* BossController = Cast<ABossCharacterBaseAiController>(RawController);
	if (!BossController)
	{
		return EStateTreeRunStatus::Failed;
	}

	AActor* Target = BossController->GetNearestTarget();
	if (!Target)
	{
		return EStateTreeRunStatus::Failed;
	}

	APawn* BossPawn = BossController->GetPawn();
	if (!BossPawn)
	{
		return EStateTreeRunStatus::Failed;
	}

	const float Dist = FVector::Dist(BossPawn->GetActorLocation(), Target->GetActorLocation());
	if (Dist <= Data.AcceptableRadius)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	BossController->MoveToActor(Target, Data.AcceptableRadius);
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FSTTask_Boss_MoveToRange::Tick(FStateTreeExecutionContext& Context,
                                                    const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);

	AAIController* RawController = Context.GetExternalDataPtr(AIControllerHandle);
	ABossCharacterBaseAiController* BossController = Cast<ABossCharacterBaseAiController>(RawController);
	if (!BossController || !BossController->GetPawn())
	{
		return EStateTreeRunStatus::Failed;
	}

	AActor* Target = BossController->GetNearestTarget();
	if (!Target)
	{
		return EStateTreeRunStatus::Failed;
	}

	const float Dist = FVector::Dist(BossController->GetPawn()->GetActorLocation(), Target->GetActorLocation());
	UE_LOG(LogTemp, Warning, TEXT("Dist: %.1f / Radius: %.1f"), Dist, Data.AcceptableRadius);
	if (Dist < Data.AcceptableRadius)
	{
		UE_LOG(LogTemp, Warning, TEXT("Succed"));
		return EStateTreeRunStatus::Succeeded;
	}
	

	BossController->MoveToActor(Target, Data.AcceptableRadius);
	return EStateTreeRunStatus::Running;
}

void FSTTask_Boss_MoveToRange::ExitState(FStateTreeExecutionContext& Context,
                                          const FStateTreeTransitionResult& Transition) const
{
	AAIController* RawController = Context.GetExternalDataPtr(AIControllerHandle);
	ABossCharacterBaseAiController* BossController = Cast<ABossCharacterBaseAiController>(RawController);
	if (!BossController)
	{
		return;
	}

	BossController->StopMovement();
}
