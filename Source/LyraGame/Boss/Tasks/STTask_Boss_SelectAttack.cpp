#include "STTask_Boss_SelectAttack.h"
#include "StateTreeExecutionContext.h" 
#include "StateTreeLinker.h"
#include "Boss/BossCharacterBaseAiController.h"
#include "Boss/BossCharacterBase.h"

DEFINE_LOG_CATEGORY_STATIC(LogSelectAttack, Log, All);

// TODO: Link() 구현
bool FSTTask_Boss_SelectAttack::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(AIControllerHandle);
	return true;
}

// TODO: EnterState() 구현 — 가중치 계산 및 패턴 선택
EStateTreeRunStatus FSTTask_Boss_SelectAttack::EnterState(FStateTreeExecutionContext& Context,
                                                          const FStateTreeTransitionResult& Transition) const
{
	AAIController* RawController = Context.GetExternalDataPtr(AIControllerHandle);
	ABossCharacterBaseAiController * SourceController = Cast<ABossCharacterBaseAiController>(RawController);
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
		UE_LOG(LogSelectAttack, Warning, TEXT("SelectAttack: BossWeightData 없음 — BP에서 할당했는지 확인"));
		return EStateTreeRunStatus::Failed;
	}
	AActor* Target = SourceController->GetNearestTarget();
	if (!Target)
	{
		UE_LOG(LogSelectAttack, Warning, TEXT("SelectAttack: Target 없음"));
		return EStateTreeRunStatus::Failed;
	}
	float Distance = FVector::Dist(Target->GetActorLocation(), SourceBase->GetActorLocation());
	UE_LOG(LogSelectAttack, Log, TEXT("SelectAttack: 타겟 거리 = %.1f"), Distance);

	TArray<TPair<const FBossAttackEntry*,float>> Candidates;
	for (const auto& Entrie : SourceWeightData->Entries)
	{
		if (Distance > Entrie.MaxDistance && Entrie.MaxDistance != 0)
		{
			UE_LOG(LogSelectAttack, Log, TEXT("SelectAttack: [%s] 거리 초과로 제외 (%.1f > %.1f)"), *Entrie.ResultTag.ToString(), Distance, Entrie.MaxDistance);
			continue;
		}
		if (Distance < Entrie.MinDistance)
		{
			UE_LOG(LogSelectAttack, Log, TEXT("SelectAttack: [%s] 거리 미달로 제외 (%.1f < %.1f)"), *Entrie.ResultTag.ToString(), Distance, Entrie.MinDistance);
			continue;
		}
		if (SourceBase->LastAttackTag == Entrie.ResultTag)
		{
			float EffectiveWeight = Entrie.BaseWeight * Entrie.RecentUsePenalty;
			Candidates.Add({&Entrie, EffectiveWeight});
		}
		else
		{
			float EffectiveWeight = Entrie.BaseWeight;
			Candidates.Add({&Entrie, EffectiveWeight});
		}
	}
	UE_LOG(LogSelectAttack, Log, TEXT("SelectAttack: 후보 수 = %d"), Candidates.Num());
	if (Candidates.Num() == 0)
	{
		UE_LOG(LogSelectAttack, Warning, TEXT("SelectAttack: 후보 없음 — 거리 설정 확인"));
		return EStateTreeRunStatus::Failed;
	}
	float TotalWeight = 0;
	for (auto  Entrie : Candidates)
	{
		TotalWeight += Entrie.Value;
	}
	float RandomNum = FMath::FRandRange(0.f, TotalWeight);
	
	const FBossAttackEntry* SelectedEntry = nullptr; 
	if (!SelectedEntry)
	{
		SelectedEntry = Candidates.Last().Key;
	}

	float Accumulate = 0;
	for (auto Entrie : Candidates)
	{
		Accumulate += Entrie.Value;
		if (RandomNum<= Accumulate)
		{
			SelectedEntry = Entrie.Key;
			break;
		}
	}
	SourceBase->SelectedAttackTag = SelectedEntry->ResultTag;
	SourceBase->LastAttackTag = SelectedEntry->ResultTag;
	UE_LOG(LogSelectAttack, Log, TEXT("SelectAttack: 선택된 패턴 = %s"), *SelectedEntry->ResultTag.ToString());
	return EStateTreeRunStatus::Succeeded;
}

// TODO: ExitState() 구현
void FSTTask_Boss_SelectAttack::ExitState(FStateTreeExecutionContext& Context,
                                          const FStateTreeTransitionResult& Transition) const
{
}
