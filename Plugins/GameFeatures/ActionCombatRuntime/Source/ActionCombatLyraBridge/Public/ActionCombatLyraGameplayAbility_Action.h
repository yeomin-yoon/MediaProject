#pragma once

#include "AbilitySystem/Abilities/LyraGameplayAbility.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "AttributeSet.h"
#include "GameplayTagContainer.h"

#include "ActionCombatComponent.h"
#include "ActionCombatLyraGameplayAbility_Action.generated.h"

class UActionCombatComponent;
class UActionCombatMeleeTraceComponent;
struct FGameplayEventData;

UCLASS(Abstract, Blueprintable)
class ACTIONCOMBATLYRABRIDGE_API UActionCombatLyraGameplayAbility_Action : public ULyraGameplayAbility
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Action Combat|Ability")
    UActionCombatComponent* GetActionCombatComponentFromActorInfo() const;

    UFUNCTION(BlueprintPure, Category = "Action Combat|Ability")
    FActionCombatActiveActionState GetCurrentActionCombatState() const;

    UFUNCTION(BlueprintCallable, Category = "Action Combat|Ability")
    UActionCombatComponent* GetActionCombatComponentFromEventData(const FGameplayEventData& TriggerEventData) const;

    UFUNCTION(BlueprintPure, Category = "Action Combat|Ability")
    FGameplayTag GetActionTagFromEventData(const FGameplayEventData& TriggerEventData) const;

    UFUNCTION(BlueprintCallable, Category = "Action Combat|Ability")
    UActionCombatMeleeTraceComponent* FindCurrentActionMeleeTraceComponent(bool bIncludeAttachedActors = true) const;

    UFUNCTION(BlueprintCallable, Category = "Action Combat|Ability")
    UActionCombatMeleeTraceComponent* FindMeleeTraceComponentBySourceId(FName TraceSourceId, bool bIncludeAttachedActors = true) const;

    UFUNCTION(BlueprintCallable, Category = "Action Combat|Ability")
    FGameplayAbilityTargetDataHandle ConsumeCurrentActionMeleeTargetData(bool bIncludeAttachedActors = true);

    UFUNCTION(BlueprintCallable, Category = "Action Combat|Ability")
    void ConsumeCurrentActionMeleeHitResults(TArray<FHitResult>& OutHitResults, bool bIncludeAttachedActors = true);

    UFUNCTION(BlueprintCallable, Category = "Action Combat|Ability")
    FGameplayAbilityTargetDataHandle ConsumeMeleeTargetDataBySourceId(FName TraceSourceId, bool bIncludeAttachedActors = true);

    UFUNCTION(BlueprintPure, Category = "Action Combat|Ability")
    float GetAvatarAttributeValue(FGameplayAttribute Attribute, bool& bFound) const;

    UFUNCTION(BlueprintPure, Category = "Action Combat|Ability")
    float GetAttributeValueFromActor(AActor* Actor, FGameplayAttribute Attribute, bool& bFound) const;

protected:
    static FGameplayTag FindFirstCombatActionTag(const FGameplayTagContainer& TagContainer);
};
