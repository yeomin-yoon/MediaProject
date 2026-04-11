#include "Boss/Tasks/STTask_Boss_AerialShockwave.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeLinker.h"
#include "AIController.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayTagContainer.h"
#include "Boss/BossCharacterBaseAiController.h"

DEFINE_LOG_CATEGORY_STATIC(LogAerialShockwave, Log, All);

bool FSTTask_Boss_AerialShockwave::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(AIControllerHandle);
	return true;
}

EStateTreeRunStatus FSTTask_Boss_AerialShockwave::EnterState(FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	Data.bIsEnd = false;
	Data.LandingElapsed = 0.f;
	Data.AbilityEndCallbackHandle.Reset();

	AAIController* RawController = Context.GetExternalDataPtr(AIControllerHandle);
	ABossCharacterBaseAiController* BossController = Cast<ABossCharacterBaseAiController>(RawController);
	if (!BossController)
	{
		UE_LOG(LogAerialShockwave, Warning, TEXT("[AerialShockwave] EnterState: BossController 캐스팅 실패"));
		return EStateTreeRunStatus::Failed;
	}

	AActor* BossPawn = BossController->GetPawn();
	if (!BossPawn)
	{
		UE_LOG(LogAerialShockwave, Warning, TEXT("[AerialShockwave] EnterState: BossPawn nullptr"));
		return EStateTreeRunStatus::Failed;
	}

	BossController->StopMovement();

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(BossPawn);
	if (!ASC)
	{
		UE_LOG(LogAerialShockwave, Warning, TEXT("[AerialShockwave] EnterState: ASC nullptr → Failed"));
		return EStateTreeRunStatus::Failed;
	}

	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		UE_LOG(LogAerialShockwave, Log, TEXT("[AerialShockwave] 등록 GA: %s | 활성화중=%s | Tags=%s"),
			*GetNameSafe(Spec.Ability),
			Spec.IsActive() ? TEXT("true") : TEXT("false"),
			*Spec.Ability->AbilityTags.ToStringSimple());
	}

	FGameplayTagContainer ActivationTags;
	ActivationTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Boss.Attack.AerialAttack")));
	const bool bSucceeded = ASC->TryActivateAbilitiesByTag(ActivationTags);
	UE_LOG(LogAerialShockwave, Warning, TEXT("[AerialShockwave] EnterState: GA 활성화 %s"), bSucceeded ? TEXT("성공") : TEXT("실패 ← 멈춤 원인 가능성"));
	if (!bSucceeded)
	{
		UE_LOG(LogAerialShockwave, Warning, TEXT("[AerialShockwave] EnterState: 콜백 미등록 채로 Running 반환 → 보스 멈춤 발생 지점"));
		return EStateTreeRunStatus::Running;
	}

	const FGameplayTag AttackTag = FGameplayTag::RequestGameplayTag(FName("Boss.Attack.AerialAttack"));
	FInstanceDataType* DataPtr = &Data;
	Data.AbilityEndCallbackHandle = ASC->AbilityEndedCallbacks.AddLambda(
		[DataPtr, AttackTag](UGameplayAbility* EndedAbility)
		{
			if (EndedAbility && EndedAbility->AbilityTags.HasTag(AttackTag))
			{
				UE_LOG(LogAerialShockwave, Log, TEXT("[AerialShockwave] GA 종료 감지 → bIsEnd=true"));
				DataPtr->bIsEnd = true;
			}
		}
	);

	UE_LOG(LogAerialShockwave, Log, TEXT("[AerialShockwave] EnterState: 콜백 등록 완료 → Running"));
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FSTTask_Boss_AerialShockwave::Tick(FStateTreeExecutionContext& Context,
	const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);

	if (Data.bIsEnd)
	{
		Data.LandingElapsed += DeltaTime;
		UE_LOG(LogAerialShockwave, Verbose, TEXT("[AerialShockwave] Tick: 착지 후 대기 %.2f / %.2f"), Data.LandingElapsed, LandingStayDuration);
		if (Data.LandingElapsed >= LandingStayDuration)
		{
			UE_LOG(LogAerialShockwave, Log, TEXT("[AerialShockwave] Tick: 대기 완료 → Succeeded"));
			return EStateTreeRunStatus::Succeeded;
		}
	}

	return EStateTreeRunStatus::Running;
}

void FSTTask_Boss_AerialShockwave::ExitState(FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);

	if (!Data.AbilityEndCallbackHandle.IsValid())
	{
		return;
	}

	AAIController* RawController = Context.GetExternalDataPtr(AIControllerHandle);
	ABossCharacterBaseAiController* BossController = Cast<ABossCharacterBaseAiController>(RawController);
	if (!BossController) { return; }

	AActor* BossPawn = BossController->GetPawn();
	if (!BossPawn) { return; }

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(BossPawn);
	if (!ASC) { return; }

	ASC->AbilityEndedCallbacks.Remove(Data.AbilityEndCallbackHandle);
	Data.AbilityEndCallbackHandle.Reset();
}
