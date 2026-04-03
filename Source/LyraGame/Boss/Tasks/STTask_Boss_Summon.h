#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "StateTreeExecutionTypes.h"
#include "STTask_Boss_Summon.generated.h"

class AAIController;

USTRUCT()
struct FSTTask_Boss_SummonInstanceData
{
	GENERATED_BODY()

	bool bIsEnd = false;
	FDelegateHandle AbilityEndCallbackHandle;
};

USTRUCT(meta = (DisplayName = "Boss Summon"))
struct FSTTask_Boss_Summon : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FSTTask_Boss_SummonInstanceData;

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
