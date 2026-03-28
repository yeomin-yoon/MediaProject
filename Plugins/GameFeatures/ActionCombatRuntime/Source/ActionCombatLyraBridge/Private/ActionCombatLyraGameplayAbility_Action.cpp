#include "ActionCombatLyraGameplayAbility_Action.h"

#include "ActionCombatBlueprintLibrary.h"
#include "ActionCombatComponent.h"
#include "ActionCombatMeleeTraceComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameFramework/Actor.h"

UActionCombatComponent* UActionCombatLyraGameplayAbility_Action::GetActionCombatComponentFromActorInfo() const
{
    if (const AActor* AvatarActor = GetAvatarActorFromActorInfo())
    {
        return AvatarActor->FindComponentByClass<UActionCombatComponent>();
    }

    return nullptr;
}

FActionCombatActiveActionState UActionCombatLyraGameplayAbility_Action::GetCurrentActionCombatState() const
{
    if (const UActionCombatComponent* CombatComponent = GetActionCombatComponentFromActorInfo())
    {
        return CombatComponent->GetActiveActionState();
    }

    return FActionCombatActiveActionState();
}

UActionCombatComponent* UActionCombatLyraGameplayAbility_Action::GetActionCombatComponentFromEventData(const FGameplayEventData& TriggerEventData) const
{
    if (const UActionCombatComponent* EventCombatComponent = Cast<UActionCombatComponent>(TriggerEventData.OptionalObject))
    {
        return const_cast<UActionCombatComponent*>(EventCombatComponent);
    }

    return GetActionCombatComponentFromActorInfo();
}

FGameplayTag UActionCombatLyraGameplayAbility_Action::GetActionTagFromEventData(const FGameplayEventData& TriggerEventData) const
{
    const FGameplayTag EventActionTag = FindFirstCombatActionTag(TriggerEventData.InstigatorTags);
    if (EventActionTag.IsValid())
    {
        return EventActionTag;
    }

    return GetCurrentActionCombatState().ActionTag;
}

UActionCombatMeleeTraceComponent* UActionCombatLyraGameplayAbility_Action::FindCurrentActionMeleeTraceComponent(bool bIncludeAttachedActors) const
{
    const FActionCombatActiveActionState ActionState = GetCurrentActionCombatState();
    return FindMeleeTraceComponentBySourceId(ActionState.TraceSourceId, bIncludeAttachedActors);
}

UActionCombatMeleeTraceComponent* UActionCombatLyraGameplayAbility_Action::FindMeleeTraceComponentBySourceId(FName TraceSourceId, bool bIncludeAttachedActors) const
{
    return UActionCombatBlueprintLibrary::FindMeleeTraceComponent(GetAvatarActorFromActorInfo(), TraceSourceId, bIncludeAttachedActors);
}

FGameplayAbilityTargetDataHandle UActionCombatLyraGameplayAbility_Action::ConsumeCurrentActionMeleeTargetData(bool bIncludeAttachedActors)
{
    if (UActionCombatMeleeTraceComponent* TraceComponent = FindCurrentActionMeleeTraceComponent(bIncludeAttachedActors))
    {
        return TraceComponent->ConsumeRecordedTargetData();
    }

    return FGameplayAbilityTargetDataHandle();
}

void UActionCombatLyraGameplayAbility_Action::ConsumeCurrentActionMeleeHitResults(TArray<FHitResult>& OutHitResults, bool bIncludeAttachedActors)
{
    OutHitResults.Reset();

    if (UActionCombatMeleeTraceComponent* TraceComponent = FindCurrentActionMeleeTraceComponent(bIncludeAttachedActors))
    {
        TraceComponent->ConsumeRecordedHitResults(OutHitResults);
    }
}

FGameplayAbilityTargetDataHandle UActionCombatLyraGameplayAbility_Action::ConsumeMeleeTargetDataBySourceId(FName TraceSourceId, bool bIncludeAttachedActors)
{
    if (UActionCombatMeleeTraceComponent* TraceComponent = FindMeleeTraceComponentBySourceId(TraceSourceId, bIncludeAttachedActors))
    {
        return TraceComponent->ConsumeRecordedTargetData();
    }

    return FGameplayAbilityTargetDataHandle();
}

float UActionCombatLyraGameplayAbility_Action::GetAvatarAttributeValue(FGameplayAttribute Attribute, bool& bFound) const
{
    return GetAttributeValueFromActor(GetAvatarActorFromActorInfo(), Attribute, bFound);
}

float UActionCombatLyraGameplayAbility_Action::GetAttributeValueFromActor(AActor* Actor, FGameplayAttribute Attribute, bool& bFound) const
{
    bFound = false;

    if (!Actor || !Attribute.IsValid())
    {
        return 0.0f;
    }

    if (UAbilitySystemComponent* AbilitySystemComponent = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Actor))
    {
        if (AbilitySystemComponent->HasAttributeSetForAttribute(Attribute))
        {
            bFound = true;
            return AbilitySystemComponent->GetNumericAttribute(Attribute);
        }
    }

    return 0.0f;
}

FGameplayTag UActionCombatLyraGameplayAbility_Action::FindFirstCombatActionTag(const FGameplayTagContainer& TagContainer)
{
    TArray<FGameplayTag> Tags;
    TagContainer.GetGameplayTagArray(Tags);

    for (const FGameplayTag& Tag : Tags)
    {
        if (Tag.ToString().StartsWith(TEXT("Combat.Action")))
        {
            return Tag;
        }
    }

    return FGameplayTag();
}
