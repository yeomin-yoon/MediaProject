#pragma once

#include "AttributeSet.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"

#include "ActionCombatStyleData.generated.h"

class UAnimMontage;

USTRUCT(BlueprintType)
struct ACTIONCOMBATRUNTIME_API FActionCombatAttributeCost
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cost")
    FGameplayAttribute Attribute;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cost", meta = (ClampMin = "0.0"))
    float RequiredValue = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cost", meta = (ClampMin = "0.0"))
    float ConsumeValue = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cost")
    bool bBlockActionIfInsufficient = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cost")
    bool bConsumeOnActionStart = false;

    bool IsRelevant() const
    {
        return Attribute.IsValid() && (RequiredValue > 0.0f || ConsumeValue > 0.0f);
    }
};

USTRUCT(BlueprintType)
struct ACTIONCOMBATRUNTIME_API FActionCombatActionDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action")
    FGameplayTag ActionTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action")
    TObjectPtr<UAnimMontage> Montage = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action", meta = (ClampMin = "0.01"))
    float FallbackDurationSeconds = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action", meta = (ClampMin = "0.01"))
    float BasePlayRate = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float QueueWindowStartsAtNormalizedTime = 0.2f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float QueueWindowClosesAtNormalizedTime = 0.95f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ChainCommitAtNormalizedTime = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action")
    bool bAllowDodgeCancel = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float DodgeCancelStartsAtNormalizedTime = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action")
    FName TraceSourceId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action")
    FName HitWindowName = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage", meta = (ClampMin = "0.0"))
    float MotionValue = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage", meta = (ClampMin = "0.0"))
    float PoiseDamage = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage", meta = (ClampMin = "0.0"))
    float BuildupMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action", meta = (Categories = "Combat"))
    FGameplayTagContainer RequiredOwnerTags;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action", meta = (Categories = "Combat"))
    FGameplayTagContainer BlockedOwnerTags;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action")
    TArray<FActionCombatAttributeCost> ResourceCosts;

    bool HasValidActionTag() const
    {
        return ActionTag.IsValid();
    }
};

USTRUCT(BlueprintType)
struct ACTIONCOMBATRUNTIME_API FActionCombatTransitionDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition")
    FGameplayTag FromActionTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition")
    FGameplayTag CommandTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition")
    FGameplayTag ToActionTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition")
    bool bRequiresFocusActive = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition")
    bool bRequiresFocusInactive = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition")
    FGameplayTagContainer RequiredHeldInputTags;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition")
    FGameplayTagContainer BlockedHeldInputTags;

    bool Matches(const FGameplayTag& ActiveActionTag, const FGameplayTag& RequestedCommandTag, bool bFocusActive, const FGameplayTagContainer& HeldInputTags) const
    {
        const bool bFromMatches = (!FromActionTag.IsValid() && !ActiveActionTag.IsValid()) || FromActionTag == ActiveActionTag;
        const bool bCommandMatches = CommandTag == RequestedCommandTag;
        const bool bFocusStateMatches = (!bRequiresFocusActive || bFocusActive) && (!bRequiresFocusInactive || !bFocusActive);
        const bool bRequiredHeldTagsMatch = RequiredHeldInputTags.IsEmpty() || HeldInputTags.HasAllExact(RequiredHeldInputTags);
        const bool bBlockedHeldTagsMatch = BlockedHeldInputTags.IsEmpty() || !HeldInputTags.HasAnyExact(BlockedHeldInputTags);
        return bFromMatches && bCommandMatches && bFocusStateMatches && bRequiredHeldTagsMatch && bBlockedHeldTagsMatch;
    }
};

UCLASS(BlueprintType)
class ACTIONCOMBATRUNTIME_API UActionCombatStyleData : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    const FActionCombatActionDefinition* FindActionDefinition(const FGameplayTag& ActionTag) const;
    const FActionCombatTransitionDefinition* FindTransitionDefinition(const FGameplayTag& FromActionTag, const FGameplayTag& CommandTag, bool bFocusActive, const FGameplayTagContainer& HeldInputTags) const;

    UFUNCTION(BlueprintPure, Category = "Action Combat|Style")
    float GetGlobalPlayRateMultiplier() const
    {
        return GlobalPlayRateMultiplier;
    }

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Style", meta = (ClampMin = "0.01"))
    float GlobalPlayRateMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Style")
    TArray<FActionCombatActionDefinition> Actions;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Style")
    TArray<FActionCombatTransitionDefinition> Transitions;
};
