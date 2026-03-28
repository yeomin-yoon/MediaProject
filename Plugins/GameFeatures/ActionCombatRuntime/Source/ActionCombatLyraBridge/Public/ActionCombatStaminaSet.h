#pragma once

#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/LyraAttributeSet.h"

#include "ActionCombatStaminaSet.generated.h"

struct FGameplayEffectModCallbackData;
struct FGameplayAttributeData;

UCLASS()
class ACTIONCOMBATLYRABRIDGE_API UActionCombatStaminaSet : public ULyraAttributeSet
{
    GENERATED_BODY()

public:
    UActionCombatStaminaSet();

    ATTRIBUTE_ACCESSORS(UActionCombatStaminaSet, Stamina);
    ATTRIBUTE_ACCESSORS(UActionCombatStaminaSet, MaxStamina);
    ATTRIBUTE_ACCESSORS(UActionCombatStaminaSet, StaminaRegenRate);

    mutable FLyraAttributeEvent OnStaminaChanged;
    mutable FLyraAttributeEvent OnMaxStaminaChanged;
    mutable FLyraAttributeEvent OnOutOfStamina;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
    virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
    virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;
    virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

protected:
    UFUNCTION()
    void OnRep_Stamina(const FGameplayAttributeData& OldValue);

    UFUNCTION()
    void OnRep_MaxStamina(const FGameplayAttributeData& OldValue);

    UFUNCTION()
    void OnRep_StaminaRegenRate(const FGameplayAttributeData& OldValue);

private:
    void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Stamina, Category = "Action Combat|Stamina", Meta = (AllowPrivateAccess = true))
    FGameplayAttributeData Stamina;

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxStamina, Category = "Action Combat|Stamina", Meta = (AllowPrivateAccess = true))
    FGameplayAttributeData MaxStamina;

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_StaminaRegenRate, Category = "Action Combat|Stamina", Meta = (AllowPrivateAccess = true))
    FGameplayAttributeData StaminaRegenRate;

    bool bOutOfStamina = false;
    float MaxStaminaBeforeAttributeChange = 0.0f;
    float StaminaBeforeAttributeChange = 0.0f;
};
