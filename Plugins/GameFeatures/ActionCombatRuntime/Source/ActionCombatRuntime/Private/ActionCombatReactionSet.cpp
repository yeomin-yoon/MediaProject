#include "ActionCombatReactionSet.h"

#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

UActionCombatReactionSet::UActionCombatReactionSet()
    : Poise(100.0f)
    , MaxPoise(100.0f)
    , PoiseRecoveryRate(20.0f)
    , KnockdownThreshold(60.0f)
{
}

void UActionCombatReactionSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, Poise, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, MaxPoise, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, PoiseRecoveryRate, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, KnockdownThreshold, COND_None, REPNOTIFY_Always);
}

void UActionCombatReactionSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
    Super::PreAttributeBaseChange(Attribute, NewValue);
    ClampAttribute(Attribute, NewValue);
}

void UActionCombatReactionSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
    Super::PreAttributeChange(Attribute, NewValue);
    ClampAttribute(Attribute, NewValue);

    if (Attribute == GetMaxPoiseAttribute())
    {
        NewValue = FMath::Max(NewValue, 1.0f);
        if (GetPoise() > NewValue)
        {
            const_cast<ThisClass*>(this)->SetPoise(NewValue);
        }
    }
}

void UActionCombatReactionSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
    Super::PostGameplayEffectExecute(Data);

    float NewValue = 0.0f;
    if (Data.EvaluatedData.Attribute == GetPoiseAttribute())
    {
        NewValue = GetPoise();
        ClampAttribute(GetPoiseAttribute(), NewValue);
        SetPoise(NewValue);
    }
    else if (Data.EvaluatedData.Attribute == GetMaxPoiseAttribute())
    {
        NewValue = GetMaxPoise();
        ClampAttribute(GetMaxPoiseAttribute(), NewValue);
        SetMaxPoise(NewValue);
        SetPoise(FMath::Clamp(GetPoise(), 0.0f, GetMaxPoise()));
    }
    else if (Data.EvaluatedData.Attribute == GetPoiseRecoveryRateAttribute())
    {
        NewValue = GetPoiseRecoveryRate();
        ClampAttribute(GetPoiseRecoveryRateAttribute(), NewValue);
        SetPoiseRecoveryRate(NewValue);
    }
    else if (Data.EvaluatedData.Attribute == GetKnockdownThresholdAttribute())
    {
        NewValue = GetKnockdownThreshold();
        ClampAttribute(GetKnockdownThresholdAttribute(), NewValue);
        SetKnockdownThreshold(NewValue);
    }
}

void UActionCombatReactionSet::OnRep_Poise(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, Poise, OldValue);
}

void UActionCombatReactionSet::OnRep_MaxPoise(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, MaxPoise, OldValue);
}

void UActionCombatReactionSet::OnRep_PoiseRecoveryRate(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, PoiseRecoveryRate, OldValue);
}

void UActionCombatReactionSet::OnRep_KnockdownThreshold(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, KnockdownThreshold, OldValue);
}

void UActionCombatReactionSet::ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const
{
    if (Attribute == GetMaxPoiseAttribute())
    {
        NewValue = FMath::Max(NewValue, 1.0f);
        return;
    }

    if (Attribute == GetPoiseAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.0f, FMath::Max(GetMaxPoise(), 1.0f));
        return;
    }

    if ((Attribute == GetPoiseRecoveryRateAttribute()) || (Attribute == GetKnockdownThresholdAttribute()))
    {
        NewValue = FMath::Max(NewValue, 0.0f);
    }
}
