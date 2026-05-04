#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "ActionCombatAttackSnapshot.generated.h"

USTRUCT(BlueprintType)
struct ACTIONCOMBATLYRABRIDGE_API FActionCombatAttackSnapshot
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Action Combat|Attack Snapshot")
    FGameplayTag ActionTag;

    UPROPERTY(BlueprintReadOnly, Category = "Action Combat|Attack Snapshot")
    float ResolvedBaseDamage = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Action Combat|Attack Snapshot")
    float MotionValue = 1.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Action Combat|Attack Snapshot")
    float PoiseDamage = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Action Combat|Attack Snapshot")
    float BuildupMultiplier = 1.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Action Combat|Attack Snapshot")
    float StrengthValue = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Action Combat|Attack Snapshot")
    float DexterityValue = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Action Combat|Attack Snapshot")
    float IntelligenceValue = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Action Combat|Attack Snapshot")
    float FaithValue = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Action Combat|Attack Snapshot")
    float ArcaneValue = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Action Combat|Attack Snapshot")
    float CustomAttackPowerValue = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Action Combat|Attack Snapshot")
    float StrengthScaling = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Action Combat|Attack Snapshot")
    float DexterityScaling = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Action Combat|Attack Snapshot")
    float IntelligenceScaling = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Action Combat|Attack Snapshot")
    float FaithScaling = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Action Combat|Attack Snapshot")
    float ArcaneScaling = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Action Combat|Attack Snapshot")
    bool bUsesWeaponDefinition = false;

    void Reset()
    {
        *this = FActionCombatAttackSnapshot();
    }

    float ComputeStatScalingContribution() const
    {
        return (StrengthValue * StrengthScaling)
            + (DexterityValue * DexterityScaling)
            + (IntelligenceValue * IntelligenceScaling)
            + (FaithValue * FaithScaling)
            + (ArcaneValue * ArcaneScaling);
    }

    float ComputeAttackPower() const
    {
        return FMath::Max(ResolvedBaseDamage + CustomAttackPowerValue + ComputeStatScalingContribution(), 0.0f);
    }

    float ComputePreviewDamage(float HitZoneMultiplier = 1.0f) const
    {
        return FMath::Max(ComputeAttackPower() * FMath::Max(MotionValue, 0.0f) * FMath::Max(HitZoneMultiplier, 0.0f), 0.0f);
    }
};
