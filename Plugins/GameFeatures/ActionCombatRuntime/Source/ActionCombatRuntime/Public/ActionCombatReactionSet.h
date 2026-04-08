#pragma once

#include "AbilitySystemComponent.h"
#include "AttributeSet.h"

#include "ActionCombatReactionSet.generated.h"

struct FGameplayAttributeData;
struct FGameplayEffectModCallbackData;

#define ACTIONCOMBAT_ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
    GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

UCLASS(BlueprintType)
class ACTIONCOMBATRUNTIME_API UActionCombatReactionSet : public UAttributeSet
{
    GENERATED_BODY()

public:
    UActionCombatReactionSet();

    ACTIONCOMBAT_ATTRIBUTE_ACCESSORS(UActionCombatReactionSet, Poise);
    ACTIONCOMBAT_ATTRIBUTE_ACCESSORS(UActionCombatReactionSet, MaxPoise);
    ACTIONCOMBAT_ATTRIBUTE_ACCESSORS(UActionCombatReactionSet, PoiseRecoveryRate);
    ACTIONCOMBAT_ATTRIBUTE_ACCESSORS(UActionCombatReactionSet, KnockdownThreshold);

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
    virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
    virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

protected:
    UFUNCTION()
    void OnRep_Poise(const FGameplayAttributeData& OldValue);

    UFUNCTION()
    void OnRep_MaxPoise(const FGameplayAttributeData& OldValue);

    UFUNCTION()
    void OnRep_PoiseRecoveryRate(const FGameplayAttributeData& OldValue);

    UFUNCTION()
    void OnRep_KnockdownThreshold(const FGameplayAttributeData& OldValue);

private:
    void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Poise, Category = "Action Combat|Reaction", Meta = (AllowPrivateAccess = true))
    FGameplayAttributeData Poise;

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxPoise, Category = "Action Combat|Reaction", Meta = (AllowPrivateAccess = true))
    FGameplayAttributeData MaxPoise;

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_PoiseRecoveryRate, Category = "Action Combat|Reaction", Meta = (AllowPrivateAccess = true))
    FGameplayAttributeData PoiseRecoveryRate;

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_KnockdownThreshold, Category = "Action Combat|Reaction", Meta = (AllowPrivateAccess = true))
    FGameplayAttributeData KnockdownThreshold;
};

#undef ACTIONCOMBAT_ATTRIBUTE_ACCESSORS
