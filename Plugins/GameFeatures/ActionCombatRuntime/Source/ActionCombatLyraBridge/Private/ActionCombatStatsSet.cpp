#include "ActionCombatStatsSet.h"

#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

UActionCombatStatsSet::UActionCombatStatsSet()
    : Strength(10.0f)
    , Dexterity(10.0f)
    , Intelligence(10.0f)
    , Faith(10.0f)
    , Arcane(10.0f)
{
}

void UActionCombatStatsSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, Strength, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, Dexterity, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, Intelligence, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, Faith, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, Arcane, COND_None, REPNOTIFY_Always);
}

void UActionCombatStatsSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
    Super::PreAttributeBaseChange(Attribute, NewValue);
    ClampAttribute(Attribute, NewValue);
}

void UActionCombatStatsSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
    Super::PreAttributeChange(Attribute, NewValue);
    ClampAttribute(Attribute, NewValue);
}

void UActionCombatStatsSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
    Super::PostGameplayEffectExecute(Data);

    float NewValue = 0.0f;
    if (Data.EvaluatedData.Attribute == GetStrengthAttribute())
    {
        NewValue = GetStrength();
        ClampAttribute(GetStrengthAttribute(), NewValue);
        SetStrength(NewValue);
    }
    else if (Data.EvaluatedData.Attribute == GetDexterityAttribute())
    {
        NewValue = GetDexterity();
        ClampAttribute(GetDexterityAttribute(), NewValue);
        SetDexterity(NewValue);
    }
    else if (Data.EvaluatedData.Attribute == GetIntelligenceAttribute())
    {
        NewValue = GetIntelligence();
        ClampAttribute(GetIntelligenceAttribute(), NewValue);
        SetIntelligence(NewValue);
    }
    else if (Data.EvaluatedData.Attribute == GetFaithAttribute())
    {
        NewValue = GetFaith();
        ClampAttribute(GetFaithAttribute(), NewValue);
        SetFaith(NewValue);
    }
    else if (Data.EvaluatedData.Attribute == GetArcaneAttribute())
    {
        NewValue = GetArcane();
        ClampAttribute(GetArcaneAttribute(), NewValue);
        SetArcane(NewValue);
    }
}

void UActionCombatStatsSet::OnRep_Strength(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, Strength, OldValue);
}

void UActionCombatStatsSet::OnRep_Dexterity(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, Dexterity, OldValue);
}

void UActionCombatStatsSet::OnRep_Intelligence(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, Intelligence, OldValue);
}

void UActionCombatStatsSet::OnRep_Faith(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, Faith, OldValue);
}

void UActionCombatStatsSet::OnRep_Arcane(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, Arcane, OldValue);
}

void UActionCombatStatsSet::ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const
{
    if ((Attribute == GetStrengthAttribute())
        || (Attribute == GetDexterityAttribute())
        || (Attribute == GetIntelligenceAttribute())
        || (Attribute == GetFaithAttribute())
        || (Attribute == GetArcaneAttribute()))
    {
        NewValue = FMath::Max(NewValue, 0.0f);
    }
}
