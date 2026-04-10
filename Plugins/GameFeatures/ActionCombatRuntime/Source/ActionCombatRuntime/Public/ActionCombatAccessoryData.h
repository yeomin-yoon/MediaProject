#pragma once

#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "UObject/SoftObjectPtr.h"

#include "ActionCombatAccessoryData.generated.h"

class UActionCombatBlueprintLibrary;
class UAnimInstance;
class UMaterialInterface;
class USkeletalMesh;
class UStaticMesh;

UCLASS(BlueprintType)
class ACTIONCOMBATRUNTIME_API UActionCombatAccessoryData : public UPrimaryDataAsset
{
    GENERATED_BODY()

    friend class UActionCombatBlueprintLibrary;

public:
    UFUNCTION(BlueprintPure, Category = "Action Combat|Accessories")
    FGameplayTag GetSlotTag() const
    {
        return SlotTag;
    }

    UFUNCTION(BlueprintPure, Category = "Action Combat|Accessories")
    bool HasVisualAsset() const;

    bool IsCompatibleWithAppearance(const FGameplayTagContainer& AppearanceTags, FString* OutFailureReason = nullptr) const;

    const TSoftObjectPtr<UStaticMesh>& GetStaticMesh() const
    {
        return StaticMesh;
    }

    const TSoftObjectPtr<USkeletalMesh>& GetSkeletalMesh() const
    {
        return SkeletalMesh;
    }

    TSubclassOf<UAnimInstance> GetAnimClass() const
    {
        return AnimClass;
    }

    const TArray<TSoftObjectPtr<UMaterialInterface>>& GetMaterialOverrides() const
    {
        return MaterialOverrides;
    }

    FName GetSocketOverride() const
    {
        return SocketOverride;
    }

    FTransform GetRelativeTransform() const
    {
        return RelativeTransform;
    }

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Accessory", meta = (Categories = "Cosmetic.Slot"))
    FGameplayTag SlotTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Accessory", meta = (Categories = "Cosmetic"))
    FGameplayTagContainer RequiredAppearanceTags;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Accessory", meta = (Categories = "Cosmetic"))
    FGameplayTagContainer BlockedAppearanceTags;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Accessory")
    TSoftObjectPtr<UStaticMesh> StaticMesh;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Accessory")
    TSoftObjectPtr<USkeletalMesh> SkeletalMesh;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Accessory")
    TSubclassOf<UAnimInstance> AnimClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Accessory")
    TArray<TSoftObjectPtr<UMaterialInterface>> MaterialOverrides;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Accessory")
    FName SocketOverride = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Accessory")
    FTransform RelativeTransform = FTransform::Identity;
};
