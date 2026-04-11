#include "STCondition_Boss_CheckAttackTag.h"

#include "StateTreeExecutionContext.h"
#include "StateTreeLinker.h"
#include "Boss/BossCharacterBase.h"
#include "Boss/BossCharacterBaseAiController.h"

DEFINE_LOG_CATEGORY_STATIC(LogCheckAttackTag, Log, All);

bool FSTCondition_Boss_CheckAttackTag::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(AIControllerHandle);
	return true;
}

bool FSTCondition_Boss_CheckAttackTag::TestCondition(FStateTreeExecutionContext& Context) const
{
	AAIController* RawController = Context.GetExternalDataPtr(AIControllerHandle);
	ABossCharacterBaseAiController* SourceController = Cast<ABossCharacterBaseAiController>(RawController);
	if (!SourceController) return false;

	ABossCharacterBase* SourceBase = Cast<ABossCharacterBase>(SourceController->GetPawn());
	if (!SourceBase) return false;

	const FInstanceDataType& Data = Context.GetInstanceData(*this);
	bool bResult = SourceBase->SelectedAttackTag == Data.ExpectedTag;
	UE_LOG(LogCheckAttackTag, Log, TEXT("CheckAttackTag: Selected=%s / Expected=%s / 결과=%s"),
		*SourceBase->SelectedAttackTag.ToString(),
		*Data.ExpectedTag.ToString(),
		bResult ? TEXT("true") : TEXT("false"));
	return bResult;
}
