#include "STTask_Boss_SelectAttack.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeLinker.h"
#include "Boss/BossCharacterBaseAiController.h"
#include "Boss/BossCharacterBase.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogSelectAttack, Log, All);

bool FSTTask_Boss_SelectAttack::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(AIControllerHandle);
	return true;
}

EStateTreeRunStatus FSTTask_Boss_SelectAttack::EnterState(FStateTreeExecutionContext& Context,
                                                          const FStateTreeTransitionResult& Transition) const
{
	AAIController* RawController = Context.GetExternalDataPtr(AIControllerHandle);
	ABossCharacterBaseAiController* SourceController = Cast<ABossCharacterBaseAiController>(RawController);
	if (!SourceController)
	{
		UE_LOG(LogSelectAttack, Warning, TEXT("SelectAttack: SourceController 캐스팅 실패"));
		return EStateTreeRunStatus::Failed;
	}

	ABossCharacterBase* SourceBase = Cast<ABossCharacterBase>(SourceController->GetPawn());
	if (!SourceBase)
	{
		UE_LOG(LogSelectAttack, Warning, TEXT("SelectAttack: SourceBase 캐스팅 실패"));
		return EStateTreeRunStatus::Failed;
	}

	UBossAttackWeightData* SourceWeightData = SourceBase->BossWeightData;
	if (!SourceWeightData)
	{
		UE_LOG(LogSelectAttack, Warning, TEXT("SelectAttack: BossWeightData 없음"));
		return EStateTreeRunStatus::Failed;
	}


	auto Fallback = [&]()
	{
		SourceBase->SelectedAttackTag = FGameplayTag::RequestGameplayTag(FName("Boss.Behavior.MoveTo"));
		UE_LOG(LogSelectAttack, Warning, TEXT("SelectAttack: 폴백 → MoveTo(근거리 이동)"));
	};

	// 스턴 중에는 공격 선택을 하지 않고 MoveTo 폴백으로 보내, GA가 활성화되지 않도록 한다
	if (UAbilitySystemComponent* BossASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(SourceBase))
	{
		if (BossASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("Boss.State.Stunned"))))
		{
			UE_LOG(LogSelectAttack, Warning, TEXT("SelectAttack: 스턴 상태 → 공격 선택 보류, MoveTo 폴백"));
			Fallback();
			return EStateTreeRunStatus::Succeeded;
		}
	}

	AActor* Target = SourceController->GetNearestTarget();
	if (!Target)
	{
		Fallback();
		return EStateTreeRunStatus::Succeeded;
	}

	const float Distance = FVector::Dist(Target->GetActorLocation(), SourceBase->GetActorLocation());
	UE_LOG(LogSelectAttack, Log, TEXT("SelectAttack: 타겟 거리 = %.1f"), Distance);

	TArray<TPair<const FBossAttackEntry*, float>> Candidates;
	for (const auto& Entry : SourceWeightData->Entries)
	{
		if (Entry.MaxDistance != 0 && Distance > Entry.MaxDistance)
		{
			UE_LOG(LogSelectAttack, Log, TEXT("SelectAttack: [%s] 거리 초과로 제외 (%.1f > %.1f)"), *Entry.ResultTag.ToString(), Distance, Entry.MaxDistance);
			continue;
		}
		if (Distance < Entry.MinDistance)
		{
			UE_LOG(LogSelectAttack, Log, TEXT("SelectAttack: [%s] 거리 미달로 제외 (%.1f < %.1f)"), *Entry.ResultTag.ToString(), Distance, Entry.MinDistance);
			continue;
		}
		const float EffectiveWeight = (SourceBase->LastAttackTag == Entry.ResultTag)
			? Entry.BaseWeight * Entry.RecentUsePenalty
			: Entry.BaseWeight;
		Candidates.Add({&Entry, EffectiveWeight});
	}

	UE_LOG(LogSelectAttack, Log, TEXT("SelectAttack: 후보 수 = %d"), Candidates.Num());
	if (Candidates.Num() == 0)
	{
		Fallback();
		return EStateTreeRunStatus::Succeeded;
	}

	float TotalWeight = 0.f;
	for (const auto& C : Candidates) TotalWeight += C.Value;

	const float RandomNum = FMath::FRandRange(0.f, TotalWeight);
	const FBossAttackEntry* SelectedEntry = Candidates.Last().Key; 

	float Accumulate = 0.f;
	for (const auto& C : Candidates)
	{
		Accumulate += C.Value;
		if (RandomNum <= Accumulate)
		{
			SelectedEntry = C.Key;
			break;
		}
	}

	SourceBase->SelectedAttackTag = SelectedEntry->ResultTag;
	SourceBase->LastAttackTag = SelectedEntry->ResultTag;
	UE_LOG(LogSelectAttack, Log, TEXT("SelectAttack: 선택된 패턴 = %s"), *SelectedEntry->ResultTag.ToString());
	return EStateTreeRunStatus::Succeeded;
}

void FSTTask_Boss_SelectAttack::ExitState(FStateTreeExecutionContext& Context,
                                          const FStateTreeTransitionResult& Transition) const
{
}
