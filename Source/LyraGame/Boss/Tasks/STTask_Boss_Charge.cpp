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
	UE_LOG(LogBossCharge, Warning, TEXT("[Charge] GA 엔터스테이트"));
	// TODO: GA_Boss_Charge 태그로 활성화
	AAIController* RawAiController = Context.GetExternalDataPtr(AIControllerHandle);
	if (!RawAiController)
	{
		return EStateTreeRunStatus::Failed;
	}
	ABossCharacterBaseAiController* SrcController = Cast<ABossCharacterBaseAiController>(RawAiController);
	if (!SrcController)
	{
		return EStateTreeRunStatus::Failed;
	}
	AActor* RawActor = SrcController->GetPawn();
	if (!RawActor)
	{
		UE_LOG(LogBossCharge, Warning, TEXT("[Charge] EnterState: Pawn nullptr"));
		return EStateTreeRunStatus::Failed;
	}
	UAbilitySystemComponent* SrcAsc = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(RawActor);
	if (!SrcAsc)
	{
		UE_LOG(LogBossCharge, Warning, TEXT("[Charge] EnterState: ASC nullptr"));
		return EStateTreeRunStatus::Failed;
	}
	FGameplayTagContainer ActivationTags;
	ActivationTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Boss.Attack.Charge")));
	const bool bIsSucced = SrcAsc->TryActivateAbilitiesByTag(ActivationTags);
	UE_LOG(LogBossCharge, Warning, TEXT("[Charge] EnterState: GA 활성화 %s"), bIsSucced ? TEXT("성공") : TEXT("실패"));
	if (!bIsSucced)
	{
		return EStateTreeRunStatus::Failed;
	}
	const FGameplayTag ChargeTag = FGameplayTag::RequestGameplayTag(FName("Boss.Attack.Charge"));
	FInstanceDataType* DataPtr = &Data;
	Data.AbilityEndCallbackHandle = SrcAsc->AbilityEndedCallbacks.AddLambda(
	  [DataPtr, ChargeTag](UGameplayAbility* EndedAbility)
	  {
		  if (EndedAbility && EndedAbility->AbilityTags.HasTag(ChargeTag))
		  {
			  UE_LOG(LogBossCharge, Warning, TEXT("[Charge] GA 종료 감지 → bIsEnd=true"));
			  DataPtr->bIsEnd = true;
		  }
	  }
  );

	
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
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	AAIController* RawController = Context.GetExternalDataPtr(AIControllerHandle);
	ABossCharacterBaseAiController* BossController =
	Cast<ABossCharacterBaseAiController>(RawController);
	if (!BossController) return;

	AActor* BossPawn = BossController->GetPawn();
	if (!BossPawn) return;

	UAbilitySystemComponent* ASC =
	UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(BossPawn);
	if (!ASC) return;

	ASC->AbilityEndedCallbacks.Remove(Data.AbilityEndCallbackHandle);
	Data.AbilityEndCallbackHandle.Reset();

}
