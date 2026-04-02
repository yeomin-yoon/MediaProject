#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "StateTreeExecutionTypes.h"
#include "STTask_Minion_MoveToTarget.generated.h"

class AAIController;

USTRUCT()
struct FSTTask_Minion_MoveToTargetInstanceData
{
	GENERATED_BODY()

	
	UPROPERTY(EditDefaultsOnly, Category = "Minion|Move")
	float AcceptableRadius = 180.f;
};

USTRUCT(meta = (DisplayName = "Minion Move To Target"))
struct FSTTask_Minion_MoveToTarget : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FSTTask_Minion_MoveToTargetInstanceData;

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
