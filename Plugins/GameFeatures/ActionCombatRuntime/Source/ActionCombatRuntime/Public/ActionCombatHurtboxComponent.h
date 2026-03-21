#pragma once

#include "Components/BoxComponent.h"
#include "GameplayTagContainer.h"

#include "ActionCombatHurtboxComponent.generated.h"

UCLASS(Blueprintable, ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class ACTIONCOMBATRUNTIME_API UActionCombatHurtboxComponent : public UBoxComponent
{
    GENERATED_BODY()

public:
    UActionCombatHurtboxComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    UFUNCTION(BlueprintPure, Category = "Action Combat|Hurtbox")
    FGameplayTag GetHitZoneTag() const
    {
        return HitZoneTag;
    }

    UFUNCTION(BlueprintPure, Category = "Action Combat|Hurtbox")
    float GetDamageMultiplier() const
    {
        return DamageMultiplier;
    }

    UFUNCTION(BlueprintCallable, Category = "Action Combat|Hurtbox")
    void ConfigureForLyraWeaponTraces();

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hurtbox")
    FGameplayTag HitZoneTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hurtbox", meta = (ClampMin = "0.0"))
    float DamageMultiplier = 1.0f;
};
