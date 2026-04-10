#include "ActionCombatBlueprintLibrary.h"

#include "ActionCombatAccessoryData.h"
#include "ActionCombatAppearanceData.h"
#include "ActionCombatMeleeTraceComponent.h"

#include "Animation/AnimInstance.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "GameFramework/Actor.h"
#include "GameplayTagsManager.h"
#include "Materials/MaterialInterface.h"

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

    static FGameplayTag RequestTag(FName TagName, bool bErrorIfNotFound)
    {
        return TagName.IsNone() ? FGameplayTag() : UGameplayTagsManager::Get().RequestGameplayTag(TagName, bErrorIfNotFound);
    }

    static FGameplayTagContainer BuildTagContainer(const TArray<FName>& TagNames, bool bErrorIfNotFound)
    {
        FGameplayTagContainer Result;

        for (const FName& TagName : TagNames)
        {
            const FGameplayTag Tag = RequestTag(TagName, bErrorIfNotFound);
            if (Tag.IsValid())
            {
                Result.AddTag(Tag);
            }
        }

        return Result;
    }
}

FGameplayTag UActionCombatBlueprintLibrary::RequestGameplayTagByName(FName TagName, bool bErrorIfNotFound)
{
    return ActionCombatBlueprintLibrary::RequestTag(TagName, bErrorIfNotFound);
}

FGameplayTagContainer UActionCombatBlueprintLibrary::MakeGameplayTagContainerFromNames(const TArray<FName>& TagNames, bool bErrorIfNotFound)
{
    return ActionCombatBlueprintLibrary::BuildTagContainer(TagNames, bErrorIfNotFound);
}

void UActionCombatBlueprintLibrary::ConfigureAppearanceData(
    UActionCombatAppearanceData* AppearanceData,
    FName DefaultPrimaryVisualMeshComponentName,
    FName DefaultSourceMeshComponentName,
    const TArray<FName>& AppearanceTagNames,
    const TArray<FActionCombatNamedAppearanceSlotDefinition>& SlotDefinitions)
{
    if (!AppearanceData)
    {
        return;
    }

    AppearanceData->Modify();
    AppearanceData->DefaultPrimaryVisualMeshComponentName = DefaultPrimaryVisualMeshComponentName;
    AppearanceData->DefaultSourceMeshComponentName = DefaultSourceMeshComponentName;
    AppearanceData->AppearanceTags = ActionCombatBlueprintLibrary::BuildTagContainer(AppearanceTagNames, false);
    AppearanceData->Slots.Reset();

    for (const FActionCombatNamedAppearanceSlotDefinition& SlotDefinition : SlotDefinitions)
    {
        FActionCombatAppearanceSlotDefinition NewDefinition;
        NewDefinition.SlotTag = ActionCombatBlueprintLibrary::RequestTag(SlotDefinition.SlotTagName, false);
        NewDefinition.TargetComponentName = SlotDefinition.TargetComponentName;
        NewDefinition.AttachSocket = SlotDefinition.AttachSocket;

        if (NewDefinition.HasValidSlot())
        {
            AppearanceData->Slots.Add(NewDefinition);
        }
    }
}

void UActionCombatBlueprintLibrary::ConfigureAccessoryData(
    UActionCombatAccessoryData* AccessoryData,
    FName SlotTagName,
    const TArray<FName>& RequiredAppearanceTagNames,
    const TArray<FName>& BlockedAppearanceTagNames,
    UStaticMesh* StaticMesh,
    USkeletalMesh* SkeletalMesh,
    TSubclassOf<UAnimInstance> AnimClass,
    const TArray<UMaterialInterface*>& MaterialOverrides,
    FName SocketOverride,
    const FTransform& RelativeTransform)
{
    if (!AccessoryData)
    {
        return;
    }

    AccessoryData->Modify();
    AccessoryData->SlotTag = ActionCombatBlueprintLibrary::RequestTag(SlotTagName, false);
    AccessoryData->RequiredAppearanceTags = ActionCombatBlueprintLibrary::BuildTagContainer(RequiredAppearanceTagNames, false);
    AccessoryData->BlockedAppearanceTags = ActionCombatBlueprintLibrary::BuildTagContainer(BlockedAppearanceTagNames, false);
    AccessoryData->StaticMesh = StaticMesh;
    AccessoryData->SkeletalMesh = SkeletalMesh;
    AccessoryData->AnimClass = AnimClass;
    AccessoryData->MaterialOverrides.Reset();

    for (UMaterialInterface* Material : MaterialOverrides)
    {
        AccessoryData->MaterialOverrides.Add(Material);
    }

    AccessoryData->SocketOverride = SocketOverride;
    AccessoryData->RelativeTransform = RelativeTransform;
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
