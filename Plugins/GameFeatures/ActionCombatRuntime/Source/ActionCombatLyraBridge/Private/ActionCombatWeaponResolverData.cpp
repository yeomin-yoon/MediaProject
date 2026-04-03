#include "ActionCombatWeaponResolverData.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

const FActionCombatWeaponResolverEntry* UActionCombatWeaponResolverData::FindEntryForWeaponClass(const UClass* WeaponClass) const
{
    if (WeaponClass == nullptr)
    {
        return nullptr;
    }

    for (const FActionCombatWeaponResolverEntry& Entry : WeaponEntries)
    {
        if (Entry.WeaponInstanceClass == WeaponClass)
        {
            return &Entry;
        }
    }

    for (const FActionCombatWeaponResolverEntry& Entry : WeaponEntries)
    {
        if (Entry.Matches(WeaponClass))
        {
            return &Entry;
        }
    }

    return nullptr;
}

#if WITH_EDITOR
EDataValidationResult UActionCombatWeaponResolverData::IsDataValid(FDataValidationContext& Context) const
{
    EDataValidationResult Result = Super::IsDataValid(Context);
    TSet<TObjectPtr<const UClass>> SeenWeaponClasses;

    for (const FActionCombatWeaponResolverEntry& Entry : WeaponEntries)
    {
        if (!Entry.WeaponInstanceClass)
        {
            Context.AddError(FText::FromString(TEXT("Every resolver entry must define a WeaponInstanceClass.")));
            Result = EDataValidationResult::Invalid;
            continue;
        }

        if (SeenWeaponClasses.Contains(Entry.WeaponInstanceClass))
        {
            Context.AddError(FText::FromString(FString::Printf(TEXT("Duplicate resolver entry found for %s."), *GetNameSafe(Entry.WeaponInstanceClass))));
            Result = EDataValidationResult::Invalid;
        }
        else
        {
            SeenWeaponClasses.Add(Entry.WeaponInstanceClass);
        }

        if (Entry.WeaponDefinition == nullptr)
        {
            Context.AddError(FText::FromString(FString::Printf(TEXT("Resolver entry %s must reference a WeaponDefinition asset."), *GetNameSafe(Entry.WeaponInstanceClass))));
            Result = EDataValidationResult::Invalid;
        }

        if (Entry.WeaponLevel < 0)
        {
            Context.AddError(FText::FromString(FString::Printf(TEXT("Resolver entry %s must have WeaponLevel >= 0."), *GetNameSafe(Entry.WeaponInstanceClass))));
            Result = EDataValidationResult::Invalid;
        }
    }

    return Result;
}
#endif
