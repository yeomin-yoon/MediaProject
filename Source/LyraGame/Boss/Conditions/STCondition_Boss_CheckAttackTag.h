#pragma once

#include "CoreMinimal.h"
#include "StateTreeConditionBase.h"
#include "StateTreeExecutionTypes.h"
#include "GameplayTagContainer.h"
#include "STCondition_Boss_CheckAttackTag.generated.h"

class AAIController;

USTRUCT()
struct FSTCondition_Boss_CheckAttackTagInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	FGameplayTag ExpectedTag;
};

USTRUCT(meta = (DisplayName = "Boss Check Attack Tag"))
struct FSTCondition_Boss_CheckAttackTag : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FSTCondition_Boss_CheckAttackTagInstanceData;

	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual bool Link(FStateTreeLinker& Linker) override;
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;

private:
	TStateTreeExternalDataHandle<AAIController> AIControllerHandle;
};
