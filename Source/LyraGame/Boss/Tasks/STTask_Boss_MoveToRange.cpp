#include "STTask_Boss_MoveToRange.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeLinker.h"
#include "AIController.h"
#include "Boss/BossCharacterBaseAiController.h"
#include "Boss/BossCharacterBase.h"

// 항상 BossCharacterBase.AttackRange를 사용
static float GetAttackRange(AAIController* Controller)
{
	if (ABossCharacterBase* Boss = Cast<ABossCharacterBase>(Controller->GetPawn()))
	{
		return Boss->AttackRange;
	}
	return 200.f; // fallback
}

bool FSTTask_Boss_MoveToRange::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(AIControllerHandle);
	return true;
}

EStateTreeRunStatus FSTTask_Boss_MoveToRange::EnterState(FStateTreeExecutionContext& Context,
                                                          const FStateTreeTransitionResult& Transition) const
{
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

	const float Range = GetAttackRange(BossController);
	const float Dist = FVector::Dist(BossPawn->GetActorLocation(), Target->GetActorLocation());
	if (Dist <= Range)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	// 50.f = 캡슐 충돌 허용치. Range로 넘기면 NavMesh가 경계에 세워서 Tick 판정 불가.
	BossController->MoveToActor(Target, 50.f);
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FSTTask_Boss_MoveToRange::Tick(FStateTreeExecutionContext& Context,
                                                    const float DeltaTime) const
{
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

	const float Range = GetAttackRange(BossController);
	const float Dist = FVector::Dist(BossController->GetPawn()->GetActorLocation(), Target->GetActorLocation());
	if (Dist <= Range)
	{
		return EStateTreeRunStatus::Succeeded;
	}

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
