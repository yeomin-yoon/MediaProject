#pragma once

#include "AttributeSet.h"
#include "AbilitySystem/Abilities/LyraAbilityCost.h"
#include "GameplayTagContainer.h"
#include "ScalableFloat.h"

#include "ActionCombatLyraAbilityCost_Attribute.generated.h"

UCLASS(meta = (DisplayName = "Attribute"))
class ACTIONCOMBATLYRABRIDGE_API UActionCombatLyraAbilityCost_Attribute : public ULyraAbilityCost
{
    GENERATED_BODY()

public:
    UActionCombatLyraAbilityCost_Attribute();

    virtual bool CheckCost(const ULyraGameplayAbility* Ability, const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const override;
    virtual void ApplyCost(const ULyraGameplayAbility* Ability, const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) override;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Costs)
    FGameplayAttribute CostAttribute;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Costs)
    FScalableFloat Quantity;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Costs)
    FGameplayTag FailureTag;
};
