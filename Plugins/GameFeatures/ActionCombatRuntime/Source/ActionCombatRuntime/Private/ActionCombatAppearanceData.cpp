#include "ActionCombatAppearanceData.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

const FActionCombatAppearanceSlotDefinition* UActionCombatAppearanceData::FindSlotDefinition(const FGameplayTag& SlotTag) const
{
    for (const FActionCombatAppearanceSlotDefinition& Slot : Slots)
    {
        if (Slot.SlotTag == SlotTag)
        {
            return &Slot;
        }
    }

    return nullptr;
}

#if WITH_EDITOR
EDataValidationResult UActionCombatAppearanceData::IsDataValid(FDataValidationContext& Context) const
{
    EDataValidationResult Result = Super::IsDataValid(Context);

    TSet<FGameplayTag> SeenSlots;
    for (const FActionCombatAppearanceSlotDefinition& Slot : Slots)
    {
        if (!Slot.SlotTag.IsValid())
        {
            Context.AddError(FText::FromString(TEXT("Every appearance slot entry must define a valid SlotTag.")));
            Result = EDataValidationResult::Invalid;
            continue;
        }

        if (SeenSlots.Contains(Slot.SlotTag))
        {
            Context.AddError(FText::FromString(FString::Printf(TEXT("Duplicate appearance slot found: %s"), *Slot.SlotTag.ToString())));
            Result = EDataValidationResult::Invalid;
        }
        else
        {
            SeenSlots.Add(Slot.SlotTag);
        }
    }

    return Result;
}
#endif
