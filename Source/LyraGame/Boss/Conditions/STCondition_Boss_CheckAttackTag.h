#pragma once

#include "CoreMinimal.h"
#include "StateTreeConditionBase.h"
#include "StateTreeExecutionTypes.h"
#include "GameplayTagContainer.h"
#include "STCondition_Boss_CheckAttackTag.generated.h"

class AAIController;

// TODO: 에디터에서 입력받을 비교 태그 필드 추가
USTRUCT()
struct FSTCondition_Boss_CheckAttackTagInstanceData
{
	GENERATED_BODY()

	// TODO: FGameplayTag ExpectedTag 선언
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

	// TODO: BossChar->SelectedAttackTag == ExpectedTag 비교 후 반환
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;

private:
	TStateTreeExternalDataHandle<AAIController> AIControllerHandle;
};
