#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "StateTreeExecutionTypes.h"
#include "STTask_Boss_AerialShockwave.generated.h"

class AAIController;

USTRUCT()
struct FSTTask_Boss_AerialShockwaveInstanceData
{
	GENERATED_BODY()

	bool bIsEnd = false;
	FDelegateHandle AbilityEndCallbackHandle;
	float LandingElapsed = 0.f;
};

USTRUCT(meta = (DisplayName = "Boss Aerial Shockwave"))
struct FSTTask_Boss_AerialShockwave : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FSTTask_Boss_AerialShockwaveInstanceData;

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

	UPROPERTY(EditDefaultsOnly, Category = "Boss|AerialAttack")
	float LandingStayDuration = 2.5f;

private:
	TStateTreeExternalDataHandle<AAIController> AIControllerHandle;
};
