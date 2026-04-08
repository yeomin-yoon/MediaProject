#pragma once

#include "ActionCombatAttackSnapshot.h"
#include "ActionCombatLyraGameplayAbility_Action.h"
#include "ActionCombatMeleeTraceTypes.h"
#include "GameplayTagContainer.h"

#include "ActionCombatLyraGameplayAbility_MeleeEffect.generated.h"

class UAbilityTask_WaitActionCombatMeleeHit;
class UAbilityTask_WaitGameplayEvent;
class UGameplayEffect;
class UActionCombatLyraGuardComponent;
class UActionCombatMeleeTraceComponent;
class UActionCombatWeaponResolverData;
struct FActionCombatReactionResult;
struct FTimerHandle;
struct FGameplayEffectSpec;
enum class EActionCombatLyraGuardOutcome : uint8;
struct FActionCombatLyraGuardResult;

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

    UFUNCTION(BlueprintPure, Category = "Action Combat|Damage")
    FActionCombatAttackSnapshot GetCurrentActivationAttackSnapshot() const
    {
        return ActivationAttackSnapshot;
    }

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Action Combat|Damage")
    bool bUseAttackSnapshotDamageExecution = true;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Action Combat|Damage")
    TSubclassOf<UGameplayEffect> SnapshotDamageEffectClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Action Combat|Damage")
    TObjectPtr<UActionCombatWeaponResolverData> WeaponResolverData = nullptr;

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

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Action Combat|Guard")
    bool bCanBeBlocked = true;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Action Combat|Dodge")
    bool bIgnoreTargetDodgeIFrame = false;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Action Combat|Dodge", meta = (Categories = "Combat.State"))
    FGameplayTag TargetDodgeIFrameTag;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Action Combat|Guard", meta = (ClampMin = "0.0"))
    float GuardDamage = 20.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Action Combat|Guard", meta = (ClampMin = "0.0"))
    float ForcedGuardDurationSeconds = 0.35f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Action Combat|Guard", meta = (ClampMin = "0.0"))
    float GuardBreakDurationSeconds = 0.9f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Action Combat|Guard")
    bool bApplyDamageOnGuardBreakHit = false;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Action Combat|Reaction")
    bool bApplyReactionOnSuccessfulHit = true;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Action Combat|Reaction")
    bool bScaleReactionByRecordedHitMultiplier = true;

    UFUNCTION(BlueprintImplementableEvent, Category = "Action Combat|Melee", DisplayName = "On Recorded Hit")
    void K2_OnRecordedHit(AActor* HitActor, FActionCombatRecordedHit RecordedHit, int32 HitIndex);

    UFUNCTION(BlueprintImplementableEvent, Category = "Action Combat|Guard", DisplayName = "On Hit Blocked")
    void K2_OnHitBlocked(AActor* HitActor, FActionCombatRecordedHit RecordedHit, int32 HitIndex, EActionCombatLyraGuardOutcome GuardOutcome);

    UFUNCTION(BlueprintImplementableEvent, Category = "Action Combat|Dodge", DisplayName = "On Hit Dodged")
    void K2_OnHitDodged(AActor* HitActor, FActionCombatRecordedHit RecordedHit, int32 HitIndex);

    UFUNCTION(BlueprintImplementableEvent, Category = "Action Combat|Melee", DisplayName = "On Target Effect Applied")
    void K2_OnTargetEffectApplied(AActor* HitActor, FActionCombatRecordedHit RecordedHit, int32 HitIndex);

private:
    void BuildActivationAttackSnapshot();
    void ResetActivationAttackSnapshot();
    FActionCombatAttackSnapshot MakeAttackSnapshot() const;
    TSubclassOf<UGameplayEffect> ResolveDamageEffectClass() const;
    void ConfigureSnapshotDamageSpec(FGameplayEffectSpec& EffectSpec, const FActionCombatRecordedHit& RecordedHit) const;
    void StartEndAbilityTimer();
    void StopEndAbilityTimer();
    FName ResolveRequestedTraceSourceId() const;
    bool ShouldAcceptHitActor(AActor* HitActor) const;
    bool ShouldIgnoreRecordedHitFromTargetDodge(AActor* HitActor) const;
    bool ApplyEffectToRecordedHit(AActor* HitActor, UActionCombatMeleeTraceComponent* TraceComponent, const FActionCombatRecordedHit& RecordedHit) const;
    bool TryResolveGuardedHit(AActor* HitActor, const FActionCombatRecordedHit& RecordedHit, FActionCombatLyraGuardResult& OutGuardResult) const;
    void TryApplyReactionToRecordedHit(AActor* HitActor, const FActionCombatRecordedHit& RecordedHit) const;

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

    UPROPERTY(Transient)
    FActionCombatAttackSnapshot ActivationAttackSnapshot;

    TSet<TObjectKey<AActor>> HitActorsDuringActivation;
    FTimerHandle EndAbilityTimerHandle;
    bool bHasActivationAttackSnapshot = false;
};
