#include "ActionCombatBlueprintLibrary.h"

#include "ActionCombatMeleeTraceComponent.h"

#include "Abilities/GameplayAbilityTargetTypes.h"
#include "GameFramework/Actor.h"

namespace ActionCombatBlueprintLibrary
{
    static void GatherActorAndAttachments(AActor* RootActor, TArray<AActor*>& OutActors)
    {
        if (!RootActor || OutActors.Contains(RootActor))
        {
            return;
        }

        OutActors.Add(RootActor);

        TArray<AActor*> AttachedActors;
        RootActor->GetAttachedActors(AttachedActors);
        for (AActor* AttachedActor : AttachedActors)
        {
            GatherActorAndAttachments(AttachedActor, OutActors);
        }
    }
}

UActionCombatMeleeTraceComponent* UActionCombatBlueprintLibrary::FindMeleeTraceComponent(AActor* Actor, FName TraceSourceId, bool bIncludeAttachedActors)
{
    const TArray<UActionCombatMeleeTraceComponent*> Components = FindMeleeTraceComponents(Actor, TraceSourceId, bIncludeAttachedActors);
    return Components.Num() > 0 ? Components[0] : nullptr;
}

TArray<UActionCombatMeleeTraceComponent*> UActionCombatBlueprintLibrary::FindMeleeTraceComponents(AActor* Actor, FName TraceSourceId, bool bIncludeAttachedActors)
{
    TArray<UActionCombatMeleeTraceComponent*> MatchingComponents;
    if (!Actor)
    {
        return MatchingComponents;
    }

    TArray<AActor*> ActorsToSearch;
    if (bIncludeAttachedActors)
    {
        ActionCombatBlueprintLibrary::GatherActorAndAttachments(Actor, ActorsToSearch);
    }
    else
    {
        ActorsToSearch.Add(Actor);
    }

    for (AActor* SearchActor : ActorsToSearch)
    {
        TArray<UActionCombatMeleeTraceComponent*> FoundComponents;
        SearchActor->GetComponents(FoundComponents);

        for (UActionCombatMeleeTraceComponent* Component : FoundComponents)
        {
            if (!Component)
            {
                continue;
            }

            if (TraceSourceId.IsNone() || Component->GetTraceSourceId() == TraceSourceId)
            {
                MatchingComponents.Add(Component);
            }
        }
    }

    return MatchingComponents;
}

FGameplayAbilityTargetDataHandle UActionCombatBlueprintLibrary::MakeTargetDataFromHitResult(const FHitResult& HitResult)
{
    TArray<FHitResult> HitResults;
    HitResults.Add(HitResult);
    return MakeTargetDataFromHitResults(HitResults);
}

FGameplayAbilityTargetDataHandle UActionCombatBlueprintLibrary::MakeTargetDataFromHitResults(const TArray<FHitResult>& HitResults)
{
    FGameplayAbilityTargetDataHandle TargetData;

    for (const FHitResult& HitResult : HitResults)
    {
        FGameplayAbilityTargetData_SingleTargetHit* NewTargetData = new FGameplayAbilityTargetData_SingleTargetHit();
        NewTargetData->HitResult = HitResult;
        TargetData.Add(NewTargetData);
    }

    return TargetData;
}

FGameplayAbilityTargetDataHandle UActionCombatBlueprintLibrary::MakeTargetDataFromRecordedHit(const FActionCombatRecordedHit& RecordedHit)
{
    TArray<FActionCombatRecordedHit> RecordedHits;
    RecordedHits.Add(RecordedHit);
    return MakeTargetDataFromRecordedHits(RecordedHits);
}

FGameplayAbilityTargetDataHandle UActionCombatBlueprintLibrary::MakeTargetDataFromRecordedHits(const TArray<FActionCombatRecordedHit>& RecordedHits)
{
    FGameplayAbilityTargetDataHandle TargetData;

    for (const FActionCombatRecordedHit& RecordedHit : RecordedHits)
    {
        FGameplayAbilityTargetData_SingleTargetHit* NewTargetData = new FGameplayAbilityTargetData_SingleTargetHit();
        NewTargetData->HitResult = RecordedHit.HitResult;
        TargetData.Add(NewTargetData);
    }

    return TargetData;
}
