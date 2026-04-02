#include "Minion/Tasks/STTask_Minion_MoveToTarget.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeLinker.h"
#include "AIController.h"
#include "Kismet/GameplayStatics.h"

// 플레이어 폰을 타겟으로 반환 (추후 멀티 대응 시 교체)
static APawn* GetMinionTarget(const UWorld* World)
{
	APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	return PC ? PC->GetPawn() : nullptr;
}

bool FSTTask_Minion_MoveToTarget::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(AIControllerHandle);
	return true;
}

EStateTreeRunStatus FSTTask_Minion_MoveToTarget::EnterState(FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& Data = Context.GetInstanceData(*this);

	AAIController* Controller = Context.GetExternalDataPtr(AIControllerHandle);
	if (!Controller || !Controller->GetPawn())
	{
		return EStateTreeRunStatus::Failed;
	}

	APawn* Target = GetMinionTarget(Controller->GetWorld());
	if (!Target)
	{
		return EStateTreeRunStatus::Failed;
	}

	const float Dist = FVector::Dist(Controller->GetPawn()->GetActorLocation(), Target->GetActorLocation());
	if (Dist <= Data.AcceptableRadius)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	
	Controller->MoveToActor(Target, 50.f);
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FSTTask_Minion_MoveToTarget::Tick(FStateTreeExecutionContext& Context,
	const float DeltaTime) const
{
	const FInstanceDataType& Data = Context.GetInstanceData(*this);

	AAIController* Controller = Context.GetExternalDataPtr(AIControllerHandle);
	if (!Controller || !Controller->GetPawn())
	{
		return EStateTreeRunStatus::Failed;
	}

	APawn* Target = GetMinionTarget(Controller->GetWorld());
	if (!Target)
	{
		return EStateTreeRunStatus::Failed;
	}

	const float Dist = FVector::Dist(Controller->GetPawn()->GetActorLocation(), Target->GetActorLocation());
	UE_LOG(LogTemp, Warning, TEXT("[Minion] 거리: %.1f / AcceptableRadius: %.1f"), Dist, Data.AcceptableRadius);
	if (Dist <= Data.AcceptableRadius)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	return EStateTreeRunStatus::Running;
}

void FSTTask_Minion_MoveToTarget::ExitState(FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	AAIController* Controller = Context.GetExternalDataPtr(AIControllerHandle);
	if (Controller)
	{
		Controller->StopMovement();
	}
}
