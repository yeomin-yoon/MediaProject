#include "STTask_Boss_MoveToRange.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeLinker.h"
#include "AIController.h"
#include "Boss/BossCharacterBaseAiController.h"
#include "Boss/BossCharacterBase.h"

DEFINE_LOG_CATEGORY_STATIC(LogMoveToRange, Log, All);

static float GetAttackRange(AAIController* Controller)
{
	if (ABossCharacterBase* Boss = Cast<ABossCharacterBase>(Controller->GetPawn()))
	{
		return Boss->AttackRange;
	}
	return 200.f;
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
		UE_LOG(LogMoveToRange, Warning, TEXT("[MoveToRange] EnterState: BossController 캐스팅 실패"));
		return EStateTreeRunStatus::Failed;
	}

	AActor* Target = BossController->GetNearestTarget();
	if (!Target)
	{
		UE_LOG(LogMoveToRange, Warning, TEXT("[MoveToRange] EnterState: Target nullptr → 플레이어 대기 중 (Running)"));
		return EStateTreeRunStatus::Running;
	}

	APawn* BossPawn = BossController->GetPawn();
	if (!BossPawn)
	{
		UE_LOG(LogMoveToRange, Warning, TEXT("[MoveToRange] EnterState: BossPawn nullptr → Failed"));
		return EStateTreeRunStatus::Failed;
	}

	const float Range = GetAttackRange(BossController);
	const float Dist = FVector::Dist(BossPawn->GetActorLocation(), Target->GetActorLocation());
	UE_LOG(LogMoveToRange, Log, TEXT("[MoveToRange] EnterState: 현재거리=%.1f / 공격범위=%.1f"), Dist, Range);

	if (Dist <= Range)
	{
		UE_LOG(LogMoveToRange, Log, TEXT("[MoveToRange] EnterState: 이미 사정거리 안 → Succeeded"));
		return EStateTreeRunStatus::Succeeded;
	}

	UE_LOG(LogMoveToRange, Log, TEXT("[MoveToRange] EnterState: 이동 시작 → Running"));
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
		UE_LOG(LogMoveToRange, Warning, TEXT("[MoveToRange] Tick: BossController 또는 Pawn nullptr → Failed"));
		return EStateTreeRunStatus::Failed;
	}

	AActor* Target = BossController->GetNearestTarget();
	if (!Target)
	{
		UE_LOG(LogMoveToRange, Warning, TEXT("[MoveToRange] Tick: Target nullptr → 플레이어 대기 중 (Running)"));
		return EStateTreeRunStatus::Running;
	}

	const float Range = GetAttackRange(BossController);
	const float Dist = FVector::Dist(BossController->GetPawn()->GetActorLocation(), Target->GetActorLocation());
	UE_LOG(LogMoveToRange, Verbose, TEXT("[MoveToRange] Tick: 현재거리=%.1f / 공격범위=%.1f"), Dist, Range);

	if (Dist <= Range)
	{
		UE_LOG(LogMoveToRange, Log, TEXT("[MoveToRange] Tick: 사정거리 진입 (%.1f <= %.1f) → Succeeded"), Dist, Range);
		return EStateTreeRunStatus::Succeeded;
	}

	BossController->MoveToActor(Target, 50.f);
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
