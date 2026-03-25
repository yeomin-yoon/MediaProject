#include "ActionCombatLyraAbilityCost_Attribute.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystem/Abilities/LyraGameplayAbility.h"
#include "LyraGameplayTags.h"

UActionCombatLyraAbilityCost_Attribute::UActionCombatLyraAbilityCost_Attribute()
{
    Quantity.SetValue(1.0f);
    FailureTag = LyraGameplayTags::Ability_ActivateFail_Cost;
}

bool UActionCombatLyraAbilityCost_Attribute::CheckCost(const ULyraGameplayAbility* Ability, const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
    if ((Ability == nullptr) || (ActorInfo == nullptr) || !CostAttribute.IsValid())
    {
        if (OptionalRelevantTags && FailureTag.IsValid())
        {
            OptionalRelevantTags->AddTag(FailureTag);
        }

        return false;
    }

    UAbilitySystemComponent* AbilitySystemComponent = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(ActorInfo->AvatarActor.Get());
    if ((AbilitySystemComponent == nullptr) || !AbilitySystemComponent->HasAttributeSetForAttribute(CostAttribute))
    {
        if (OptionalRelevantTags && FailureTag.IsValid())
        {
            OptionalRelevantTags->AddTag(FailureTag);
        }

        return false;
    }

    const int32 AbilityLevel = Ability->GetAbilityLevel(Handle, ActorInfo);
    const float CostValue = Quantity.GetValueAtLevel(AbilityLevel);
    const float CurrentValue = AbilitySystemComponent->GetNumericAttribute(CostAttribute);
    const bool bCanAffordCost = CurrentValue >= CostValue;

    if (!bCanAffordCost && OptionalRelevantTags && FailureTag.IsValid())
    {
        OptionalRelevantTags->AddTag(FailureTag);
    }

    return bCanAffordCost;
}

void UActionCombatLyraAbilityCost_Attribute::ApplyCost(const ULyraGameplayAbility* Ability, const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
    if ((Ability == nullptr) || (ActorInfo == nullptr) || !ActorInfo->IsNetAuthority() || !CostAttribute.IsValid())
    {
        return;
    }

    if (UAbilitySystemComponent* AbilitySystemComponent = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(ActorInfo->AvatarActor.Get()))
    {
        const int32 AbilityLevel = Ability->GetAbilityLevel(Handle, ActorInfo);
        const float CostValue = Quantity.GetValueAtLevel(AbilityLevel);
        AbilitySystemComponent->ApplyModToAttribute(CostAttribute, EGameplayModOp::Additive, -CostValue);
    }
}
