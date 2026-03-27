#include "ActionCombatStyleData.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

const FActionCombatActionDefinition* UActionCombatStyleData::FindActionDefinition(const FGameplayTag& ActionTag) const
{
    for (const FActionCombatActionDefinition& Action : Actions)
    {
        if (Action.ActionTag == ActionTag)
        {
            return &Action;
        }
    }

    return nullptr;
}

#if WITH_EDITOR
EDataValidationResult UActionCombatStyleData::IsDataValid(FDataValidationContext& Context) const
{
    EDataValidationResult Result = Super::IsDataValid(Context);

    if (GlobalPlayRateMultiplier <= 0.0f)
    {
        Context.AddError(FText::FromString(TEXT("GlobalPlayRateMultiplier must be greater than zero.")));
        Result = EDataValidationResult::Invalid;
    }

    TSet<FGameplayTag> SeenActionTags;
    for (const FActionCombatActionDefinition& Action : Actions)
    {
        if (!Action.ActionTag.IsValid())
        {
            Context.AddError(FText::FromString(TEXT("Every action entry must define a valid ActionTag.")));
            Result = EDataValidationResult::Invalid;
            continue;
        }

        if (SeenActionTags.Contains(Action.ActionTag))
        {
            Context.AddError(FText::FromString(FString::Printf(TEXT("Duplicate action tag found: %s"), *Action.ActionTag.ToString())));
            Result = EDataValidationResult::Invalid;
        }
        else
        {
            SeenActionTags.Add(Action.ActionTag);
        }

        if (Action.FallbackDurationSeconds <= 0.0f)
        {
            Context.AddError(FText::FromString(FString::Printf(TEXT("Action %s must have FallbackDurationSeconds > 0."), *Action.ActionTag.ToString())));
            Result = EDataValidationResult::Invalid;
        }

        if (Action.BasePlayRate <= 0.0f)
        {
            Context.AddError(FText::FromString(FString::Printf(TEXT("Action %s must have BasePlayRate > 0."), *Action.ActionTag.ToString())));
            Result = EDataValidationResult::Invalid;
        }

        if (Action.QueueWindowStartsAtNormalizedTime > Action.QueueWindowClosesAtNormalizedTime)
        {
            Context.AddError(FText::FromString(FString::Printf(TEXT("Action %s has QueueWindowStartsAtNormalizedTime after QueueWindowClosesAtNormalizedTime."), *Action.ActionTag.ToString())));
            Result = EDataValidationResult::Invalid;
        }

        if (Action.ChainCommitAtNormalizedTime < Action.QueueWindowStartsAtNormalizedTime)
        {
            Context.AddError(FText::FromString(FString::Printf(TEXT("Action %s has ChainCommitAtNormalizedTime before QueueWindowStartsAtNormalizedTime."), *Action.ActionTag.ToString())));
            Result = EDataValidationResult::Invalid;
        }

        for (const FActionCombatAttributeCost& ResourceCost : Action.ResourceCosts)
        {
            if ((ResourceCost.RequiredValue <= 0.0f) && (ResourceCost.ConsumeValue <= 0.0f))
            {
                continue;
            }

            if (!ResourceCost.Attribute.IsValid())
            {
                Context.AddError(FText::FromString(FString::Printf(TEXT("Action %s defines a resource cost without a valid attribute."), *Action.ActionTag.ToString())));
                Result = EDataValidationResult::Invalid;
            }
        }
    }

    for (const FActionCombatTransitionDefinition& Transition : Transitions)
    {
        if (!Transition.CommandTag.IsValid())
        {
            Context.AddError(FText::FromString(TEXT("Every transition entry must define a valid CommandTag.")));
            Result = EDataValidationResult::Invalid;
        }

        if (!Transition.ToActionTag.IsValid())
        {
            Context.AddError(FText::FromString(TEXT("Every transition entry must define a valid ToActionTag.")));
            Result = EDataValidationResult::Invalid;
        }

        if (Transition.bRequiresFocusActive && Transition.bRequiresFocusInactive)
        {
            Context.AddError(FText::FromString(FString::Printf(TEXT("Transition to %s cannot require focus to be both active and inactive."), *Transition.ToActionTag.ToString())));
            Result = EDataValidationResult::Invalid;
        }
    }

    return Result;
}
#endif

const FActionCombatTransitionDefinition* UActionCombatStyleData::FindTransitionDefinition(const FGameplayTag& FromActionTag, const FGameplayTag& CommandTag, bool bFocusActive, const FGameplayTagContainer& HeldInputTags) const
{
    for (const FActionCombatTransitionDefinition& Transition : Transitions)
    {
        if (Transition.Matches(FromActionTag, CommandTag, bFocusActive, HeldInputTags))
        {
            return &Transition;
        }
    }

    return nullptr;
}
