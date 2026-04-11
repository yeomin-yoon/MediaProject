#include "Boss/Tasks/STTask_Boss_Charge.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeLinker.h"
#include "AIController.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Boss/BossCharacterBaseAiController.h"

DEFINE_LOG_CATEGORY_STATIC(LogBossCharge, Log, All);

bool FSTTask_Boss_Charge::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(AIControllerHandle);
	return true;
}

EStateTreeRunStatus FSTTask_Boss_Charge::EnterState(FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	Data.bIsEnd = false;
	Data.AbilityEndCallbackHandle.Reset();

	// TODO: GA_Boss_Charge 태그로 활성화
	// TODO: GA 종료 콜백 등록 (NormalAttack 참고)

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FSTTask_Boss_Charge::Tick(FStateTreeExecutionContext& Context,
	const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);

	if (Data.bIsEnd)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	return EStateTreeRunStatus::Running;
}

void FSTTask_Boss_Charge::ExitState(FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	// TODO: 콜백 해제 (NormalAttack 참고)
}
