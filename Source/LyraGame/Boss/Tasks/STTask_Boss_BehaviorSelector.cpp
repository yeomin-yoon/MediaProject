#include "STTask_Boss_BehaviorSelector.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeLinker.h"
#include "AIController.h"
#include "Boss/BossCharacterBaseAiController.h"
#include "Boss/BossCharacterBase.h"
#include "AbilitySystemComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogBossAI, Log, All);

bool FSTTask_Boss_BehaviorSelector::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(AIControllerHandle);
	return true;
}

EStateTreeRunStatus FSTTask_Boss_BehaviorSelector::EnterState(FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	Data.SelectorState = EBossSelectorState::Idle;
	Data.ActiveActionIndex = -1;
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FSTTask_Boss_BehaviorSelector::Tick(FStateTreeExecutionContext& Context,
	const float DeltaTime) const
{
	// TODO: 구현
	return EStateTreeRunStatus::Running;
}

void FSTTask_Boss_BehaviorSelector::ExitState(FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	// TODO: StopMovement
}

int32 FSTTask_Boss_BehaviorSelector::SelectAction(const TArray<FBossActionData>& Actions,
	float Distance, float CurrentTime) const
{
	// TODO: 필터링 → 가중치 계산 → 룰렛 휠
	return -1;
}
