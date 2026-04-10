#include "ActionCombatAccessoryData.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

bool UActionCombatAccessoryData::HasVisualAsset() const
{
    return !StaticMesh.IsNull() || !SkeletalMesh.IsNull();
}

bool UActionCombatAccessoryData::IsCompatibleWithAppearance(const FGameplayTagContainer& AppearanceTags, FString* OutFailureReason) const
{
    const bool bRequiredTagsMatch = RequiredAppearanceTags.IsEmpty() || AppearanceTags.HasAll(RequiredAppearanceTags);
    if (!bRequiredTagsMatch)
    {
        if (OutFailureReason)
        {
            *OutFailureReason = FString::Printf(TEXT("Accessory %s is missing one or more required appearance tags."), *GetPathNameSafe(this));
        }

        return false;
    }

    const bool bBlockedTagsMatch = BlockedAppearanceTags.IsEmpty() || !AppearanceTags.HasAny(BlockedAppearanceTags);
    if (!bBlockedTagsMatch)
    {
        if (OutFailureReason)
        {
            *OutFailureReason = FString::Printf(TEXT("Accessory %s is blocked by the target appearance tags."), *GetPathNameSafe(this));
        }

        return false;
    }

    return true;
}

#if WITH_EDITOR
EDataValidationResult UActionCombatAccessoryData::IsDataValid(FDataValidationContext& Context) const
{
    EDataValidationResult Result = Super::IsDataValid(Context);

    if (!SlotTag.IsValid())
    {
        Context.AddError(FText::FromString(TEXT("Accessory data must define a valid SlotTag.")));
        Result = EDataValidationResult::Invalid;
    }

    const bool bHasStaticMesh = !StaticMesh.IsNull();
    const bool bHasSkeletalMesh = !SkeletalMesh.IsNull();
    if (bHasStaticMesh == bHasSkeletalMesh)
    {
        Context.AddError(FText::FromString(TEXT("Accessory data must define exactly one visual mesh: either StaticMesh or SkeletalMesh.")));
        Result = EDataValidationResult::Invalid;
    }

    if (AnimClass && !bHasSkeletalMesh)
    {
        Context.AddError(FText::FromString(TEXT("AnimClass can only be used when SkeletalMesh is set.")));
        Result = EDataValidationResult::Invalid;
    }

    return Result;
}
#endif
