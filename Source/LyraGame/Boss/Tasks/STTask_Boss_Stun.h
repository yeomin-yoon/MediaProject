#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "StateTreeExecutionTypes.h"
#include "STTask_Boss_Stun.generated.h"

class AAIController;
class ABossCharacterBaseAiController;

USTRUCT()
struct FSTTask_Boss_StunInstanceData
{
	GENERATED_BODY()

	// 디버그/안전망용 누적 시간 (Tick에서 폴링 로그 등에 활용 가능)
	float ElapsedTime = 0.f;
};

USTRUCT(meta = (DisplayName = "Boss Stun"))
struct FSTTask_Boss_Stun : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FSTTask_Boss_StunInstanceData;

	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual bool Link(FStateTreeLinker& Linker) override;

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context,
	                                       const FStateTreeTransitionResult& Transition) const override;

	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context,
	                                 const float DeltaTime) const override;

	virtual void ExitState(FStateTreeExecutionContext& Context,
	                       const FStateTreeTransitionResult& Transition) const override;

private:
	TStateTreeExternalDataHandle<AAIController> AIControllerHandle;
};
