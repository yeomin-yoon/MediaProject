#pragma once

#include "GameplayEffectExecutionCalculation.h"

#include "ActionCombatWeaponDamageExecution.generated.h"

struct FGameplayEffectSpec;
struct FGameplayTag;

UCLASS()
class ACTIONCOMBATLYRABRIDGE_API UActionCombatWeaponDamageExecution : public UGameplayEffectExecutionCalculation
{
    GENERATED_BODY()

public:
    UActionCombatWeaponDamageExecution();

    virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;

private:
    static float GetSpecMagnitude(const FGameplayEffectSpec& Spec, const FGameplayTag& Tag, float DefaultValue);
};
