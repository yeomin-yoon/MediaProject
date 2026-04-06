#include "ActionCombatGameplayEffect_WeaponDamage.h"

#include "ActionCombatWeaponDamageExecution.h"

UActionCombatGameplayEffect_WeaponDamage::UActionCombatGameplayEffect_WeaponDamage()
{
    DurationPolicy = EGameplayEffectDurationType::Instant;

    FGameplayEffectExecutionDefinition& ExecutionDefinition = Executions.AddDefaulted_GetRef();
    ExecutionDefinition.CalculationClass = UActionCombatWeaponDamageExecution::StaticClass();
}
