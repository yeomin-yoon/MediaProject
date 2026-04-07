#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "StateTreeExecutionTypes.h"
#include "STTask_Boss_SelectAttack.generated.h"

class AAIController;

// TODO: InstanceData 필드 채우기
USTRUCT()
struct FSTTask_Boss_SelectAttackInstanceData
{
	GENERATED_BODY()
};

USTRUCT(meta = (DisplayName = "Boss Select Attack"))
struct FSTTask_Boss_SelectAttack : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FSTTask_Boss_SelectAttackInstanceData;

	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual bool Link(FStateTreeLinker& Linker) override;

	// TODO: 가중치 선택 로직 구현
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context,
	                                       const FStateTreeTransitionResult& Transition) const override;

	virtual void ExitState(FStateTreeExecutionContext& Context,
	                       const FStateTreeTransitionResult& Transition) const override;

private:
	TStateTreeExternalDataHandle<AAIController> AIControllerHandle;
};
