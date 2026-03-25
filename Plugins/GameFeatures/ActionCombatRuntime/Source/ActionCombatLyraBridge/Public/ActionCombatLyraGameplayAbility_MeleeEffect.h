#pragma once

#include "ActionCombatLyraGameplayAbility_Action.h"
#include "ActionCombatMeleeTraceTypes.h"
#include "GameplayTagContainer.h"

#include "ActionCombatLyraGameplayAbility_MeleeEffect.generated.h"

class UAbilityTask_WaitActionCombatMeleeHit;
class UAbilityTask_WaitGameplayEvent;
class UGameplayEffect;
class UActionCombatMeleeTraceComponent;
struct FTimerHandle;

UCLASS(Abstract, Blueprintable)
class ACTIONCOMBATLYRABRIDGE_API UActionCombatLyraGameplayAbility_MeleeEffect : public UActionCombatLyraGameplayAbility_Action
{
    GENERATED_BODY()

public:
    UActionCombatLyraGameplayAbility_MeleeEffect(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
    virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

    UFUNCTION(BlueprintCallable, Category = "Action Combat|Ability")
    void EndActiveMeleeAbility();

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Action Combat|Melee")
    TSubclassOf<UGameplayEffect> TargetEffectClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Action Combat|Melee")
    FGameplayAttribute BaseDamageAttribute;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Action Combat|Melee")
    float FallbackBaseDamage = 10.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Action Combat|Melee")
    FName TraceSourceIdOverride = NAME_None;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Action Combat|Melee")
    bool bIncludeAttachedActors = true;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Action Combat|Melee")
    bool bAllowMultipleHitsPerActorPerActivation = false;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Action Combat|Melee")
    bool bEndAbilityAfterFirstSuccessfulHit = false;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Action Combat|Melee")
    float MaxActiveDurationSeconds = 1.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Action Combat|Melee")
    bool bWaitForActionEndedEvent = false;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Action Combat|Melee", meta = (Categories = "GameplayEvent"))
    FGameplayTag ActionEndedEventTag;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Action Combat|Melee")
    bool bAddHitZoneTagToEffectSpec = true;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Action Combat|Melee")
    bool bWriteFinalDamageToLyraSetByCallerDamage = true;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Action Combat|Melee")
    FGameplayTag DamageMultiplierSetByCallerTag;

    UFUNCTION(BlueprintImplementableEvent, Category = "Action Combat|Melee", DisplayName = "On Recorded Hit")
    void K2_OnRecordedHit(AActor* HitActor, FActionCombatRecordedHit RecordedHit, int32 HitIndex);

    UFUNCTION(BlueprintImplementableEvent, Category = "Action Combat|Melee", DisplayName = "On Target Effect Applied")
    void K2_OnTargetEffectApplied(AActor* HitActor, FActionCombatRecordedHit RecordedHit, int32 HitIndex);

private:
    void StartEndAbilityTimer();
    void StopEndAbilityTimer();
    FName ResolveRequestedTraceSourceId() const;
    bool ShouldAcceptHitActor(AActor* HitActor) const;
    bool ApplyEffectToRecordedHit(AActor* HitActor, UActionCombatMeleeTraceComponent* TraceComponent, const FActionCombatRecordedHit& RecordedHit) const;

    UFUNCTION()
    void HandleRecordedHit(UActionCombatMeleeTraceComponent* TraceComponent, FActionCombatRecordedHit RecordedHit, int32 HitIndex);

    UFUNCTION()
    void HandleActionEndedEvent(FGameplayEventData Payload);

    UFUNCTION()
    void HandleTimedAbilityEnd();

    UPROPERTY(Transient)
    TObjectPtr<UAbilityTask_WaitActionCombatMeleeHit> WaitForMeleeHitTask;

    UPROPERTY(Transient)
    TObjectPtr<UAbilityTask_WaitGameplayEvent> WaitForActionEndedEventTask;

    TSet<TObjectKey<AActor>> HitActorsDuringActivation;
    FTimerHandle EndAbilityTimerHandle;
};
