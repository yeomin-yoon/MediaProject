#pragma once

#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/LyraAttributeSet.h"

#include "ActionCombatStatsSet.generated.h"

struct FGameplayAttributeData;
struct FGameplayEffectModCallbackData;

UCLASS(BlueprintType)
class ACTIONCOMBATLYRABRIDGE_API UActionCombatStatsSet : public ULyraAttributeSet
{
    GENERATED_BODY()

public:
    UActionCombatStatsSet();

    ATTRIBUTE_ACCESSORS(UActionCombatStatsSet, Strength);
    ATTRIBUTE_ACCESSORS(UActionCombatStatsSet, Dexterity);
    ATTRIBUTE_ACCESSORS(UActionCombatStatsSet, Intelligence);
    ATTRIBUTE_ACCESSORS(UActionCombatStatsSet, Faith);
    ATTRIBUTE_ACCESSORS(UActionCombatStatsSet, Arcane);

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
    virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
    virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

protected:
    UFUNCTION()
    void OnRep_Strength(const FGameplayAttributeData& OldValue);

    UFUNCTION()
    void OnRep_Dexterity(const FGameplayAttributeData& OldValue);

    UFUNCTION()
    void OnRep_Intelligence(const FGameplayAttributeData& OldValue);

    UFUNCTION()
    void OnRep_Faith(const FGameplayAttributeData& OldValue);

    UFUNCTION()
    void OnRep_Arcane(const FGameplayAttributeData& OldValue);

private:
    void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Strength, Category = "Action Combat|Stats", Meta = (AllowPrivateAccess = true))
    FGameplayAttributeData Strength;

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Dexterity, Category = "Action Combat|Stats", Meta = (AllowPrivateAccess = true))
    FGameplayAttributeData Dexterity;

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Intelligence, Category = "Action Combat|Stats", Meta = (AllowPrivateAccess = true))
    FGameplayAttributeData Intelligence;

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Faith, Category = "Action Combat|Stats", Meta = (AllowPrivateAccess = true))
    FGameplayAttributeData Faith;

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Arcane, Category = "Action Combat|Stats", Meta = (AllowPrivateAccess = true))
    FGameplayAttributeData Arcane;
};
