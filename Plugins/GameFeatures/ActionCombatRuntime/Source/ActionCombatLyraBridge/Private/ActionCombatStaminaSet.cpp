#include "ActionCombatStaminaSet.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

UActionCombatStaminaSet::UActionCombatStaminaSet()
    : Stamina(100.0f)
    , MaxStamina(100.0f)
    , StaminaRegenRate(15.0f)
{
}

void UActionCombatStaminaSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, Stamina, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, MaxStamina, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, StaminaRegenRate, COND_None, REPNOTIFY_Always);
}

void UActionCombatStaminaSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
    Super::PreAttributeBaseChange(Attribute, NewValue);
    ClampAttribute(Attribute, NewValue);
}

void UActionCombatStaminaSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
    Super::PreAttributeChange(Attribute, NewValue);
    ClampAttribute(Attribute, NewValue);
}

void UActionCombatStaminaSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
    Super::PostAttributeChange(Attribute, OldValue, NewValue);

    if ((Attribute == GetMaxStaminaAttribute()) && (GetStamina() > NewValue))
    {
        if (UAbilitySystemComponent* AbilitySystemComponent = GetOwningAbilitySystemComponent())
        {
            AbilitySystemComponent->ApplyModToAttribute(GetStaminaAttribute(), EGameplayModOp::Override, NewValue);
        }
    }

    if (bOutOfStamina && (GetStamina() > 0.0f))
    {
        bOutOfStamina = false;
    }
}

void UActionCombatStaminaSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
    Super::PostGameplayEffectExecute(Data);

    AActor* Instigator = nullptr;
    AActor* Causer = nullptr;
    if (const FGameplayEffectContextHandle ContextHandle = Data.EffectSpec.GetContext(); ContextHandle.IsValid())
    {
        Instigator = ContextHandle.GetOriginalInstigator();
        Causer = ContextHandle.GetEffectCauser();
    }

    if (Data.EvaluatedData.Attribute == GetStaminaAttribute())
    {
        SetStamina(FMath::Clamp(GetStamina(), 0.0f, GetMaxStamina()));
    }
    else if (Data.EvaluatedData.Attribute == GetMaxStaminaAttribute())
    {
        SetMaxStamina(FMath::Max(GetMaxStamina(), 1.0f));
    }
    else if (Data.EvaluatedData.Attribute == GetStaminaRegenRateAttribute())
    {
        SetStaminaRegenRate(FMath::Max(GetStaminaRegenRate(), 0.0f));
    }

    if (GetMaxStamina() != MaxStaminaBeforeAttributeChange)
    {
        OnMaxStaminaChanged.Broadcast(Instigator, Causer, &Data.EffectSpec, Data.EvaluatedData.Magnitude, MaxStaminaBeforeAttributeChange, GetMaxStamina());
    }

    if (GetStamina() != StaminaBeforeAttributeChange)
    {
        OnStaminaChanged.Broadcast(Instigator, Causer, &Data.EffectSpec, Data.EvaluatedData.Magnitude, StaminaBeforeAttributeChange, GetStamina());
    }

    if ((GetStamina() <= 0.0f) && !bOutOfStamina)
    {
        OnOutOfStamina.Broadcast(Instigator, Causer, &Data.EffectSpec, Data.EvaluatedData.Magnitude, StaminaBeforeAttributeChange, GetStamina());
    }

    bOutOfStamina = (GetStamina() <= 0.0f);
    MaxStaminaBeforeAttributeChange = GetMaxStamina();
    StaminaBeforeAttributeChange = GetStamina();
}

void UActionCombatStaminaSet::OnRep_Stamina(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, Stamina, OldValue);

    const float CurrentValue = GetStamina();
    const float EstimatedMagnitude = CurrentValue - OldValue.GetCurrentValue();
    OnStaminaChanged.Broadcast(nullptr, nullptr, nullptr, EstimatedMagnitude, OldValue.GetCurrentValue(), CurrentValue);

    if ((CurrentValue <= 0.0f) && !bOutOfStamina)
    {
        OnOutOfStamina.Broadcast(nullptr, nullptr, nullptr, EstimatedMagnitude, OldValue.GetCurrentValue(), CurrentValue);
    }

    bOutOfStamina = (CurrentValue <= 0.0f);
}

void UActionCombatStaminaSet::OnRep_MaxStamina(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, MaxStamina, OldValue);

    OnMaxStaminaChanged.Broadcast(nullptr, nullptr, nullptr, GetMaxStamina() - OldValue.GetCurrentValue(), OldValue.GetCurrentValue(), GetMaxStamina());
}

void UActionCombatStaminaSet::OnRep_StaminaRegenRate(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, StaminaRegenRate, OldValue);
}

void UActionCombatStaminaSet::ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const
{
    if (Attribute == GetStaminaAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxStamina());
    }
    else if (Attribute == GetMaxStaminaAttribute())
    {
        NewValue = FMath::Max(NewValue, 1.0f);
    }
    else if (Attribute == GetStaminaRegenRateAttribute())
    {
        NewValue = FMath::Max(NewValue, 0.0f);
    }
}
