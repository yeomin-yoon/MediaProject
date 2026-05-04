#include "ActionCombatWeaponDamageExecution.h"

#include "ActionCombatLyraBridgeTags.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/LyraHealthSet.h"
#include "AbilitySystem/LyraGameplayEffectContext.h"
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"
#include "GameFramework/Actor.h"
#include "ActionCombatRuntimeLog.h"
#include "Teams/LyraTeamSubsystem.h"
#include "Yeomin/Inventory/CustomStatusAttributeSet.h"

UActionCombatWeaponDamageExecution::UActionCombatWeaponDamageExecution()
{
}

float UActionCombatWeaponDamageExecution::GetSpecMagnitude(const FGameplayEffectSpec& Spec, const FGameplayTag& Tag, float DefaultValue)
{
    return Spec.GetSetByCallerMagnitude(Tag, false, DefaultValue);
}

void UActionCombatWeaponDamageExecution::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
#if WITH_SERVER_CODE
    const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();
    const FLyraGameplayEffectContext* TypedContext = FLyraGameplayEffectContext::ExtractEffectContext(Spec.GetContext());
    const AActor* EffectCauser = TypedContext ? TypedContext->GetEffectCauser() : nullptr;
    const FHitResult* HitActorResult = TypedContext ? TypedContext->GetHitResult() : nullptr;

    AActor* HitActor = nullptr;
    if (HitActorResult)
    {
        HitActor = HitActorResult->HitObjectHandle.FetchActor();
    }

    UAbilitySystemComponent* TargetAbilitySystemComponent = ExecutionParams.GetTargetAbilitySystemComponent();
    if ((HitActor == nullptr) && (TargetAbilitySystemComponent != nullptr))
    {
        HitActor = TargetAbilitySystemComponent->GetAvatarActor_Direct();
    }

    float DamageInteractionAllowedMultiplier = 1.0f;
    if (HitActor != nullptr)
    {
        if (ULyraTeamSubsystem* TeamSubsystem = HitActor->GetWorld()->GetSubsystem<ULyraTeamSubsystem>())
        {
            DamageInteractionAllowedMultiplier = TeamSubsystem->CanCauseDamage(EffectCauser, HitActor) ? 1.0f : 0.0f;
        }
    }

    const float BaseDamage = 1 + FMath::Max(GetSpecMagnitude(Spec, ActionCombatLyraBridgeTags::SetByCaller_Attack_BaseDamage, 0.0f), 0.0f);
    const float MotionValue = FMath::Max(GetSpecMagnitude(Spec, ActionCombatLyraBridgeTags::SetByCaller_Attack_MotionValue, 1.0f), 0.0f);
    const float DamageMultiplier = FMath::Max(GetSpecMagnitude(Spec, ActionCombatLyraBridgeTags::SetByCaller_DamageMultiplier, 1.0f), 0.0f);
    const float CustomAttackPower = FMath::Max(GetSpecMagnitude(Spec, ActionCombatLyraBridgeTags::SetByCaller_Attack_CustomAttackPower, 0.0f), 0.0f);

    const float StrengthContribution = GetSpecMagnitude(Spec, ActionCombatLyraBridgeTags::SetByCaller_Attack_Strength, 0.0f)
        * GetSpecMagnitude(Spec, ActionCombatLyraBridgeTags::SetByCaller_Attack_StrengthScaling, 0.0f);
    const float DexterityContribution = GetSpecMagnitude(Spec, ActionCombatLyraBridgeTags::SetByCaller_Attack_Dexterity, 0.0f)
        * GetSpecMagnitude(Spec, ActionCombatLyraBridgeTags::SetByCaller_Attack_DexterityScaling, 0.0f);
    const float IntelligenceContribution = GetSpecMagnitude(Spec, ActionCombatLyraBridgeTags::SetByCaller_Attack_Intelligence, 0.0f)
        * GetSpecMagnitude(Spec, ActionCombatLyraBridgeTags::SetByCaller_Attack_IntelligenceScaling, 0.0f);
    const float FaithContribution = GetSpecMagnitude(Spec, ActionCombatLyraBridgeTags::SetByCaller_Attack_Faith, 0.0f)
        * GetSpecMagnitude(Spec, ActionCombatLyraBridgeTags::SetByCaller_Attack_FaithScaling, 0.0f);
    const float ArcaneContribution = GetSpecMagnitude(Spec, ActionCombatLyraBridgeTags::SetByCaller_Attack_Arcane, 0.0f)
        * GetSpecMagnitude(Spec, ActionCombatLyraBridgeTags::SetByCaller_Attack_ArcaneScaling, 0.0f);

    const float StatScaledDamage = StrengthContribution + DexterityContribution + IntelligenceContribution + FaithContribution + ArcaneContribution;
    float TargetDamageReduction = 0.0f;
    if ((TargetAbilitySystemComponent != nullptr) && TargetAbilitySystemComponent->HasAttributeSetForAttribute(UCustomStatusAttributeSet::GetDamageReductionAttribute()))
    {
        TargetDamageReduction = FMath::Max(TargetAbilitySystemComponent->GetNumericAttribute(UCustomStatusAttributeSet::GetDamageReductionAttribute()), 0.0f);
    }

    const float DamageBeforeReduction = 1 + FMath::Max((BaseDamage + CustomAttackPower + StatScaledDamage) * MotionValue * DamageMultiplier * DamageInteractionAllowedMultiplier, 0.0f);
    const float DamageDone = FMath::Max(DamageBeforeReduction - TargetDamageReduction, 0.0f);

    UE_LOG(
        LogActionCombatRuntime,
        Log,
        TEXT("ActionCombatWeaponDamageExecution: Effect=%s Causer=%s Target=%s BaseDamage=%.2f CustomAttackPower=%.2f StatScaledDamage=%.2f MotionValue=%.2f DamageMultiplier=%.2f TeamMultiplier=%.2f DamageReduction=%.2f FinalDamage=%.2f"),
        *GetPathNameSafe(Spec.Def),
        *GetPathNameSafe(EffectCauser),
        *GetPathNameSafe(HitActor),
        BaseDamage,
        CustomAttackPower,
        StatScaledDamage,
        MotionValue,
        DamageMultiplier,
        DamageInteractionAllowedMultiplier,
        TargetDamageReduction,
        DamageDone);

    if (DamageDone > 0.0f)
    {
        OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(ULyraHealthSet::GetDamageAttribute(), EGameplayModOp::Additive, DamageDone));
    }
#endif
}
