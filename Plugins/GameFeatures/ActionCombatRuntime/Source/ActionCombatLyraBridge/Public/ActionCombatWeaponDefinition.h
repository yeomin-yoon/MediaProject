#pragma once

#include "Engine/DataAsset.h"

#include "ActionCombatWeaponDefinition.generated.h"

UCLASS(BlueprintType)
class ACTIONCOMBATLYRABRIDGE_API UActionCombatWeaponDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category = "Action Combat|Weapon")
    float GetResolvedBaseDamage(int32 WeaponLevel) const
    {
        const int32 SafeWeaponLevel = FMath::Max(WeaponLevel, 0);
        return FMath::Max(BasePhysicalDamage * (1.0f + (SafeWeaponLevel * DamageMultiplierPerLevel)), 0.0f);
    }

    UFUNCTION(BlueprintPure, Category = "Action Combat|Weapon")
    float GetStrengthScaling() const
    {
        return FMath::Max(StrengthScaling, 0.0f);
    }

    UFUNCTION(BlueprintPure, Category = "Action Combat|Weapon")
    float GetDexterityScaling() const
    {
        return FMath::Max(DexterityScaling, 0.0f);
    }

    UFUNCTION(BlueprintPure, Category = "Action Combat|Weapon")
    float GetIntelligenceScaling() const
    {
        return FMath::Max(IntelligenceScaling, 0.0f);
    }

    UFUNCTION(BlueprintPure, Category = "Action Combat|Weapon")
    float GetFaithScaling() const
    {
        return FMath::Max(FaithScaling, 0.0f);
    }

    UFUNCTION(BlueprintPure, Category = "Action Combat|Weapon")
    float GetArcaneScaling() const
    {
        return FMath::Max(ArcaneScaling, 0.0f);
    }

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage", meta = (ClampMin = "0.0"))
    float BasePhysicalDamage = 10.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage|Upgrade", meta = (ClampMin = "0.0"))
    float DamageMultiplierPerLevel = 0.05f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage|Scaling", meta = (ClampMin = "0.0"))
    float StrengthScaling = 0.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage|Scaling", meta = (ClampMin = "0.0"))
    float DexterityScaling = 0.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage|Scaling", meta = (ClampMin = "0.0"))
    float IntelligenceScaling = 0.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage|Scaling", meta = (ClampMin = "0.0"))
    float FaithScaling = 0.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage|Scaling", meta = (ClampMin = "0.0"))
    float ArcaneScaling = 0.0f;
};
