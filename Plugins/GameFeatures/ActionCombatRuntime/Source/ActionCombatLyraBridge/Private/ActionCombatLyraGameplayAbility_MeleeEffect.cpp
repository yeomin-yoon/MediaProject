#include "ActionCombatLyraGameplayAbility_MeleeEffect.h"

#include "ActionCombatLyraBridgeTags.h"
#include "ActionCombatLyraGuardComponent.h"
#include "AbilityTask_WaitActionCombatMeleeHit.h"
#include "ActionCombatMeleeTraceComponent.h"
#include "ActionCombatRuntimeLog.h"

#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "LyraGameplayTags.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"

UActionCombatLyraGameplayAbility_MeleeEffect::UActionCombatLyraGameplayAbility_MeleeEffect(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
    DamageMultiplierSetByCallerTag = ActionCombatLyraBridgeTags::SetByCaller_DamageMultiplier;
    TargetDodgeIFrameTag = ActionCombatLyraBridgeTags::Combat_State_Dodge_IFrame;
}

void UActionCombatLyraGameplayAbility_MeleeEffect::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    HitActorsDuringActivation.Reset();

    UE_LOG(
        LogActionCombatRuntime,
        Log,
        TEXT("[MeleeAbility:%s] Activated Ability=%s TraceSource=%s TargetEffect=%s"),
        *GetPathNameSafe(GetAvatarActorFromActorInfo()),
        *GetNameSafe(GetClass()),
        *ResolveRequestedTraceSourceId().ToString(),
        *GetNameSafe(TargetEffectClass));

    WaitForMeleeHitTask = UAbilityTask_WaitActionCombatMeleeHit::WaitActionCombatMeleeHit(this, ResolveRequestedTraceSourceId(), bIncludeAttachedActors, false, false);
    if (WaitForMeleeHitTask)
    {
        WaitForMeleeHitTask->OnHit.AddDynamic(this, &ThisClass::HandleRecordedHit);
        WaitForMeleeHitTask->ReadyForActivation();
    }

    if (bWaitForActionEndedEvent && ActionEndedEventTag.IsValid())
    {
        WaitForActionEndedEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, ActionEndedEventTag, nullptr, true, true);
        if (WaitForActionEndedEventTask)
        {
            WaitForActionEndedEventTask->EventReceived.AddDynamic(this, &ThisClass::HandleActionEndedEvent);
            WaitForActionEndedEventTask->ReadyForActivation();
        }
    }

    StartEndAbilityTimer();
}

void UActionCombatLyraGameplayAbility_MeleeEffect::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
    StopEndAbilityTimer();
    HitActorsDuringActivation.Reset();
    WaitForMeleeHitTask = nullptr;
    WaitForActionEndedEventTask = nullptr;

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UActionCombatLyraGameplayAbility_MeleeEffect::EndActiveMeleeAbility()
{
    if (IsActive())
    {
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
    }
}

void UActionCombatLyraGameplayAbility_MeleeEffect::StartEndAbilityTimer()
{
    if ((MaxActiveDurationSeconds <= 0.0f) || (GetWorld() == nullptr))
    {
        return;
    }

    GetWorld()->GetTimerManager().SetTimer(EndAbilityTimerHandle, this, &ThisClass::HandleTimedAbilityEnd, MaxActiveDurationSeconds, false);
}

void UActionCombatLyraGameplayAbility_MeleeEffect::StopEndAbilityTimer()
{
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(EndAbilityTimerHandle);
    }
}

FName UActionCombatLyraGameplayAbility_MeleeEffect::ResolveRequestedTraceSourceId() const
{
    if (!TraceSourceIdOverride.IsNone())
    {
        return TraceSourceIdOverride;
    }

    return GetCurrentActionCombatState().TraceSourceId;
}

bool UActionCombatLyraGameplayAbility_MeleeEffect::ShouldAcceptHitActor(AActor* HitActor) const
{
    if (HitActor == nullptr)
    {
        return false;
    }

    return bAllowMultipleHitsPerActorPerActivation || !HitActorsDuringActivation.Contains(TObjectKey<AActor>(HitActor));
}

bool UActionCombatLyraGameplayAbility_MeleeEffect::ShouldIgnoreRecordedHitFromTargetDodge(AActor* HitActor) const
{
    if (bIgnoreTargetDodgeIFrame || (HitActor == nullptr) || !TargetDodgeIFrameTag.IsValid())
    {
        return false;
    }

    const UAbilitySystemComponent* TargetAbilitySystemComponent = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(HitActor);
    return TargetAbilitySystemComponent != nullptr && TargetAbilitySystemComponent->HasMatchingGameplayTag(TargetDodgeIFrameTag);
}

bool UActionCombatLyraGameplayAbility_MeleeEffect::ApplyEffectToRecordedHit(AActor* HitActor, UActionCombatMeleeTraceComponent* TraceComponent, const FActionCombatRecordedHit& RecordedHit) const
{
    if ((HitActor == nullptr) || (TargetEffectClass == nullptr))
    {
        UE_LOG(
            LogActionCombatRuntime,
            Warning,
            TEXT("[MeleeAbility:%s] ApplyEffect skipped HitActor=%s TargetEffect=%s"),
            *GetPathNameSafe(GetAvatarActorFromActorInfo()),
            *GetNameSafe(HitActor),
            *GetNameSafe(TargetEffectClass));
        return false;
    }

    UAbilitySystemComponent* TargetAbilitySystemComponent = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(HitActor);
    if (TargetAbilitySystemComponent == nullptr)
    {
        UE_LOG(
            LogActionCombatRuntime,
            Warning,
            TEXT("[MeleeAbility:%s] ApplyEffect skipped because target ASC was not found for HitActor=%s"),
            *GetPathNameSafe(GetAvatarActorFromActorInfo()),
            *GetNameSafe(HitActor));
        return false;
    }

    FGameplayEffectSpecHandle EffectSpecHandle = MakeOutgoingGameplayEffectSpec(TargetEffectClass, GetAbilityLevel());
    FGameplayEffectSpec* EffectSpec = EffectSpecHandle.Data.Get();
    if (EffectSpec == nullptr)
    {
        UE_LOG(
            LogActionCombatRuntime,
            Warning,
            TEXT("[MeleeAbility:%s] ApplyEffect skipped because effect spec could not be created for %s"),
            *GetPathNameSafe(GetAvatarActorFromActorInfo()),
            *GetNameSafe(TargetEffectClass));
        return false;
    }

    FGameplayEffectContextHandle EffectContext = EffectSpec->GetContext();
    EffectContext.AddHitResult(RecordedHit.HitResult, true);
    EffectContext.AddSourceObject(TraceComponent ? TraceComponent->GetOwner() : GetAvatarActorFromActorInfo());
    EffectSpec->SetContext(EffectContext);

    if (bAddHitZoneTagToEffectSpec && RecordedHit.HitZoneTag.IsValid())
    {
        EffectSpec->AddDynamicAssetTag(RecordedHit.HitZoneTag);
    }

    if (bWriteFinalDamageToLyraSetByCallerDamage)
    {
        bool bFoundBaseDamage = false;
        float BaseDamage = FallbackBaseDamage;
        if (BaseDamageAttribute.IsValid())
        {
            BaseDamage = GetAvatarAttributeValue(BaseDamageAttribute, bFoundBaseDamage);
            if (!bFoundBaseDamage)
            {
                BaseDamage = FallbackBaseDamage;
            }
        }

        EffectSpec->SetSetByCallerMagnitude(LyraGameplayTags::SetByCaller_Damage, BaseDamage * RecordedHit.DamageMultiplier);

        UE_LOG(
            LogActionCombatRuntime,
            Log,
            TEXT("[MeleeAbility:%s] PreparedDamage HitActor=%s BaseDamage=%.2f Multiplier=%.2f FinalDamage=%.2f"),
            *GetPathNameSafe(GetAvatarActorFromActorInfo()),
            *GetNameSafe(HitActor),
            BaseDamage,
            RecordedHit.DamageMultiplier,
            BaseDamage * RecordedHit.DamageMultiplier);
    }

    if (DamageMultiplierSetByCallerTag.IsValid())
    {
        EffectSpec->SetSetByCallerMagnitude(DamageMultiplierSetByCallerTag, RecordedHit.DamageMultiplier);
    }

    TargetAbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*EffectSpec);

    UE_LOG(
        LogActionCombatRuntime,
        Log,
        TEXT("[MeleeAbility:%s] AppliedEffect HitActor=%s Effect=%s"),
        *GetPathNameSafe(GetAvatarActorFromActorInfo()),
        *GetNameSafe(HitActor),
        *GetNameSafe(TargetEffectClass));
    return true;
}

bool UActionCombatLyraGameplayAbility_MeleeEffect::TryResolveGuardedHit(AActor* HitActor, const FActionCombatRecordedHit& RecordedHit, FActionCombatLyraGuardResult& OutGuardResult) const
{
    if ((HitActor == nullptr) || !bCanBeBlocked)
    {
        return false;
    }

    UActionCombatLyraGuardComponent* GuardComponent = UActionCombatLyraGuardComponent::FindGuardComponent(HitActor);
    if (!GuardComponent)
    {
        return false;
    }

    FActionCombatLyraIncomingGuardHit IncomingHit;
    IncomingHit.Attacker = GetAvatarActorFromActorInfo();
    IncomingHit.HitResult = RecordedHit.HitResult;
    IncomingHit.bBlockable = bCanBeBlocked;
    IncomingHit.GuardDamage = GuardDamage;
    IncomingHit.ForcedGuardDurationSeconds = ForcedGuardDurationSeconds;
    IncomingHit.GuardBreakDurationSeconds = GuardBreakDurationSeconds;
    return GuardComponent->TryResolveIncomingHit(IncomingHit, OutGuardResult);
}

void UActionCombatLyraGameplayAbility_MeleeEffect::HandleRecordedHit(UActionCombatMeleeTraceComponent* TraceComponent, FActionCombatRecordedHit RecordedHit, int32 HitIndex)
{
    AActor* HitActor = RecordedHit.HitResult.GetActor();
    K2_OnRecordedHit(HitActor, RecordedHit, HitIndex);

    UE_LOG(
        LogActionCombatRuntime,
        Log,
        TEXT("[MeleeAbility:%s] HandleRecordedHit HitActor=%s Component=%s Index=%d Authority=%s"),
        *GetPathNameSafe(GetAvatarActorFromActorInfo()),
        *GetNameSafe(HitActor),
        *GetNameSafe(RecordedHit.HitResult.GetComponent()),
        HitIndex,
        (CurrentActorInfo && CurrentActorInfo->IsNetAuthority()) ? TEXT("true") : TEXT("false"));

    if ((CurrentActorInfo == nullptr) || !CurrentActorInfo->IsNetAuthority())
    {
        return;
    }

    if (!ShouldAcceptHitActor(HitActor))
    {
        UE_LOG(
            LogActionCombatRuntime,
            Log,
            TEXT("[MeleeAbility:%s] Rejected duplicate or invalid hit for HitActor=%s"),
            *GetPathNameSafe(GetAvatarActorFromActorInfo()),
            *GetNameSafe(HitActor));
        return;
    }

    if (ShouldIgnoreRecordedHitFromTargetDodge(HitActor))
    {
        K2_OnHitDodged(HitActor, RecordedHit, HitIndex);

        UE_LOG(
            LogActionCombatRuntime,
            Log,
            TEXT("[MeleeAbility:%s] DodgeIgnored HitActor=%s DodgeTag=%s"),
            *GetPathNameSafe(GetAvatarActorFromActorInfo()),
            *GetNameSafe(HitActor),
            *TargetDodgeIFrameTag.ToString());
        return;
    }

    FActionCombatLyraGuardResult GuardResult;
    if (TryResolveGuardedHit(HitActor, RecordedHit, GuardResult))
    {
        HitActorsDuringActivation.Add(TObjectKey<AActor>(HitActor));
        K2_OnHitBlocked(HitActor, RecordedHit, HitIndex, GuardResult.Outcome);

        UE_LOG(
            LogActionCombatRuntime,
            Log,
            TEXT("[MeleeAbility:%s] GuardResolved HitActor=%s Outcome=%d GuardDamage=%.2f RemainingResource=%.2f"),
            *GetPathNameSafe(GetAvatarActorFromActorInfo()),
            *GetNameSafe(HitActor),
            static_cast<int32>(GuardResult.Outcome),
            GuardResult.AppliedGuardDamage,
            GuardResult.ResourceAfter);

        const bool bShouldSkipHealthDamage = (GuardResult.Outcome == EActionCombatLyraGuardOutcome::Blocked)
            || ((GuardResult.Outcome == EActionCombatLyraGuardOutcome::GuardBroken) && !bApplyDamageOnGuardBreakHit);

        if (bShouldSkipHealthDamage)
        {
            if (bEndAbilityAfterFirstSuccessfulHit)
            {
                EndActiveMeleeAbility();
            }

            return;
        }
    }

    if (ApplyEffectToRecordedHit(HitActor, TraceComponent, RecordedHit))
    {
        HitActorsDuringActivation.Add(TObjectKey<AActor>(HitActor));
        K2_OnTargetEffectApplied(HitActor, RecordedHit, HitIndex);

        if (bEndAbilityAfterFirstSuccessfulHit)
        {
            EndActiveMeleeAbility();
        }
    }
}

void UActionCombatLyraGameplayAbility_MeleeEffect::HandleActionEndedEvent(FGameplayEventData Payload)
{
    EndActiveMeleeAbility();
}

void UActionCombatLyraGameplayAbility_MeleeEffect::HandleTimedAbilityEnd()
{
    EndActiveMeleeAbility();
}
