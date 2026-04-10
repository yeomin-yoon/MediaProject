#pragma once

#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"

#include "ActionCombatAppearanceData.generated.h"

class UActionCombatBlueprintLibrary;

USTRUCT(BlueprintType)
struct ACTIONCOMBATRUNTIME_API FActionCombatAppearanceSlotDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance", meta = (Categories = "Cosmetic.Slot"))
    FGameplayTag SlotTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
    FName TargetComponentName = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
    FName AttachSocket = NAME_None;

    bool HasValidSlot() const
    {
        return SlotTag.IsValid();
    }
};

UCLASS(BlueprintType)
class ACTIONCOMBATRUNTIME_API UActionCombatAppearanceData : public UPrimaryDataAsset
{
    GENERATED_BODY()

    friend class UActionCombatBlueprintLibrary;

public:
    const FActionCombatAppearanceSlotDefinition* FindSlotDefinition(const FGameplayTag& SlotTag) const;

    UFUNCTION(BlueprintPure, Category = "Action Combat|Appearance")
    FGameplayTagContainer GetAppearanceTags() const
    {
        return AppearanceTags;
    }

    UFUNCTION(BlueprintPure, Category = "Action Combat|Appearance")
    FName GetDefaultPrimaryVisualMeshComponentName() const
    {
        return DefaultPrimaryVisualMeshComponentName;
    }

    UFUNCTION(BlueprintPure, Category = "Action Combat|Appearance")
    FName GetDefaultSourceMeshComponentName() const
    {
        return DefaultSourceMeshComponentName;
    }

    UFUNCTION(BlueprintPure, Category = "Action Combat|Appearance")
    TArray<FActionCombatAppearanceSlotDefinition> GetSlots() const
    {
        return Slots;
    }

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
    FName DefaultPrimaryVisualMeshComponentName = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
    FName DefaultSourceMeshComponentName = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance", meta = (Categories = "Cosmetic"))
    FGameplayTagContainer AppearanceTags;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
    TArray<FActionCombatAppearanceSlotDefinition> Slots;
};
