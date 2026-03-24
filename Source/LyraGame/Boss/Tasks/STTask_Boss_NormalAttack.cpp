#include "Boss/Tasks/STTask_Boss_NormalAttack.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeLinker.h"
#include "AIController.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayTagContainer.h"
#include "Boss/BossCharacterBaseAiController.h"

bool FSTTask_Boss_NormalAttack::Link(FStateTreeLinker& Linker)
{
	// TODO: AIControllerHandle 등록 (MoveToRange 참고)
	Linker.LinkExternalData(AIControllerHandle);
	return true;
}

EStateTreeRunStatus FSTTask_Boss_NormalAttack::EnterState(FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	UE_LOG(LogTemp,Warning,TEXT("공격"));
	// TODO: Data 초기화 (bAbilityEnded = false, AbilityEndedHandle.Reset())
	Data.AbilityEndCallbackHandle.Reset();
	Data.bIsEnd = false;
	// TODO: AIController 가져오기 → nullptr 체크 → Failed
	AAIController* RawController = Context.GetExternalDataPtr(AIControllerHandle);
	ABossCharacterBaseAiController* BossController = Cast<ABossCharacterBaseAiController>(RawController);
	if (!BossController)
	{
		return EStateTreeRunStatus::Failed;
	}
	// TODO: BossPawn 가져오기 → nullptr 체크 → Failed
	AActor* BossPawn = BossController->GetPawn();
	if (!BossPawn)
	{
		return EStateTreeRunStatus::Failed;
	}

	// 공격 시작 시 이동 중단
	BossController->StopMovement();

	UAbilitySystemComponent* AbilitySystemComponent = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(BossPawn);
	if (!AbilitySystemComponent)
	{
		return EStateTreeRunStatus::Failed;
	}

	// Boss.Attack.BaseAttack 태그를 가진 GA 발동 (BP 서브클래스도 태그로 찾을 수 있음)
	FGameplayTagContainer ActivationTags;
	ActivationTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Boss.Attack.BaseAttack")));
	const bool isSucced = AbilitySystemComponent->TryActivateAbilitiesByTag(ActivationTags);
	UE_LOG(LogTemp, Warning, TEXT("NormalAttack Activate: %s"), isSucced ? TEXT("SUCCESS") : TEXT("FAILED"));
	if (!isSucced)
	{
		return EStateTreeRunStatus::Failed;
	}

	// GA 완료 시 bIsEnd 플래그 설정 (태그로 확인)
	const FGameplayTag AttackTag = FGameplayTag::RequestGameplayTag(FName("Boss.Attack.BaseAttack"));
	FInstanceDataType* DataPtr = &Data;
	Data.AbilityEndCallbackHandle = AbilitySystemComponent->AbilityEndedCallbacks.AddLambda(
		[DataPtr, AttackTag](UGameplayAbility* EndedAbility)
		{
			if (EndedAbility && EndedAbility->AbilityTags.HasTag(AttackTag))
			{
				DataPtr->bIsEnd = true;
			}
		}
	);


	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FSTTask_Boss_NormalAttack::Tick(FStateTreeExecutionContext& Context,
	const float DeltaTime) const
{
	// TODO: InstanceData 가져오기
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	// TODO: bAbilityEnded가 true이면 Succeeded 반환
	if (Data.bIsEnd)
	{
		return EStateTreeRunStatus::Succeeded;
	}
	
	return EStateTreeRunStatus::Running;
}

void FSTTask_Boss_NormalAttack::ExitState(FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	// TODO: InstanceData 가져오기
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	// TODO: AbilityEndedHandle이 유효하지 않으면 return
	if (!Data.AbilityEndCallbackHandle.IsValid())
	{
		return;
	}

	// TODO: AIController → BossPawn → ASC 가져오기
	AAIController* RawController = Context.GetExternalDataPtr(AIControllerHandle);
	ABossCharacterBaseAiController* BossController = Cast<ABossCharacterBaseAiController>(RawController);
	AActor* BossPawn = BossController->GetPawn();
	if (!BossPawn)
	{
		return;
	}
	UAbilitySystemComponent* AbilitySystemComponent = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(BossPawn);
	// TODO: ASC->AbilityEndedCallbacks.Remove(Data.AbilityEndedHandle) 호출
	AbilitySystemComponent->AbilityEndedCallbacks.Remove(Data.AbilityEndCallbackHandle);
	// TODO: Data.AbilityEndedHandle.Reset()
	Data.AbilityEndCallbackHandle.Reset();
}
