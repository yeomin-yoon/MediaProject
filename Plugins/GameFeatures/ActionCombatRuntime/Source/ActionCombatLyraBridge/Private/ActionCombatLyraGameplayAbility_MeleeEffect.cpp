#include "ActionCombatLyraGameplayAbility_MeleeEffect.h"

#include "ActionCombatGameplayEffect_WeaponDamage.h"
#include "ActionCombatLyraBridgeTags.h"
#include "ActionCombatLyraEquipmentResolver.h"
#include "ActionCombatLyraGuardComponent.h"
#include "ActionCombatReactionComponent.h"
#include "ActionCombatStatsSet.h"
#include "ActionCombatWeaponDefinition.h"
#include "ActionCombatWeaponResolverData.h"
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
#include "Teams/LyraTeamSubsystem.h"
#include "Yeomin/Inventory/CustomStatusAttributeSet.h"

UActionCombatLyraGameplayAbility_MeleeEffect::UActionCombatLyraGameplayAbility_MeleeEffect(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
    SnapshotDamageEffectClass = UActionCombatGameplayEffect_WeaponDamage::StaticClass();
    DamageMultiplierSetByCallerTag = ActionCombatLyraBridgeTags::SetByCaller_DamageMultiplier;
    TargetDodgeIFrameTag = ActionCombatLyraBridgeTags::Combat_State_Dodge_IFrame;
    ActionEndedEventTag = FGameplayTag::RequestGameplayTag(TEXT("Combat.GameplayEvent.Action.Ended"), false);
    EnsureDefaultGameplayEventTriggers();
}

void UActionCombatLyraGameplayAbility_MeleeEffect::PostLoad()
{
    Super::PostLoad();
    EnsureDefaultGameplayEventTriggers();
}

void UActionCombatLyraGameplayAbility_MeleeEffect::EnsureDefaultGameplayEventTriggers()
{
    const FGameplayTag ActionStartedEventTag = FGameplayTag::RequestGameplayTag(TEXT("Combat.GameplayEvent.Action.Started"), false);
    if (!ActionStartedEventTag.IsValid())
    {
        return;
    }

    for (const FAbilityTriggerData& TriggerData : AbilityTriggers)
    {
        if ((TriggerData.TriggerSource == EGameplayAbilityTriggerSource::GameplayEvent)
            && TriggerData.TriggerTag.MatchesTagExact(ActionStartedEventTag))
        {
            return;
        }
    }

    FAbilityTriggerData TriggerData;
    TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
    TriggerData.TriggerTag = ActionStartedEventTag;
    AbilityTriggers.Add(TriggerData);
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
    BuildActivationAttackSnapshot();

    UE_LOG(
        LogActionCombatRuntime,
        Log,
        TEXT("[MeleeAbility:%s] Activated Ability=%s TraceSource=%s TargetEffect=%s"),
        *GetPathNameSafe(GetAvatarActorFromActorInfo()),
        *GetNameSafe(GetClass()),
        *ResolveRequestedTraceSourceId().ToString(),
        *GetNameSafe(ResolveDamageEffectClass()));

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
    ResetActivationAttackSnapshot();

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
    const TSubclassOf<UGameplayEffect> ResolvedEffectClass = ResolveDamageEffectClass();
    if ((HitActor == nullptr) || (ResolvedEffectClass == nullptr))
    {
        UE_LOG(
            LogActionCombatRuntime,
            Warning,
            TEXT("[MeleeAbility:%s] ApplyEffect skipped HitActor=%s TargetEffect=%s"),
            *GetPathNameSafe(GetAvatarActorFromActorInfo()),
            *GetNameSafe(HitActor),
            *GetNameSafe(ResolvedEffectClass));
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

    FGameplayEffectSpecHandle EffectSpecHandle = MakeOutgoingGameplayEffectSpec(ResolvedEffectClass, GetAbilityLevel());
    FGameplayEffectSpec* EffectSpec = EffectSpecHandle.Data.Get();
    if (EffectSpec == nullptr)
    {
        UE_LOG(
            LogActionCombatRuntime,
            Warning,
            TEXT("[MeleeAbility:%s] ApplyEffect skipped because effect spec could not be created for %s"),
            *GetPathNameSafe(GetAvatarActorFromActorInfo()),
            *GetNameSafe(ResolvedEffectClass));
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

    if (bUseAttackSnapshotDamageExecution)
    {
        ConfigureSnapshotDamageSpec(*EffectSpec, RecordedHit);
    }
    else if (bWriteFinalDamageToLyraSetByCallerDamage)
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

    if (!bUseAttackSnapshotDamageExecution && DamageMultiplierSetByCallerTag.IsValid())
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
        *GetNameSafe(ResolvedEffectClass));
    return true;
}

void UActionCombatLyraGameplayAbility_MeleeEffect::BuildActivationAttackSnapshot()
{
    ActivationAttackSnapshot = MakeAttackSnapshot();
    bHasActivationAttackSnapshot = true;

    UE_LOG(
        LogActionCombatRuntime,
        Log,
        TEXT("[MeleeAbility:%s] AttackSnapshot Action=%s BaseDamage=%.2f MotionValue=%.2f CustomAttackPower=%.2f StatScaling=%.2f WeaponDefinition=%s"),
        *GetPathNameSafe(GetAvatarActorFromActorInfo()),
        *ActivationAttackSnapshot.ActionTag.ToString(),
        ActivationAttackSnapshot.ResolvedBaseDamage,
        ActivationAttackSnapshot.MotionValue,
        ActivationAttackSnapshot.CustomAttackPowerValue,
        ActivationAttackSnapshot.ComputeStatScalingContribution(),
        ActivationAttackSnapshot.bUsesWeaponDefinition ? TEXT("true") : TEXT("false"));
}

void UActionCombatLyraGameplayAbility_MeleeEffect::ResetActivationAttackSnapshot()
{
    ActivationAttackSnapshot.Reset();
    bHasActivationAttackSnapshot = false;
}

FActionCombatAttackSnapshot UActionCombatLyraGameplayAbility_MeleeEffect::MakeAttackSnapshot() const
{
    FActionCombatAttackSnapshot Snapshot;
    const FActionCombatActiveActionState ActionState = GetCurrentActionCombatState();
    Snapshot.ActionTag = ActionState.ActionTag;
    Snapshot.MotionValue = FMath::Max(ActionState.MotionValue, 0.0f);
    Snapshot.PoiseDamage = FMath::Max(ActionState.PoiseDamage, 0.0f);
    Snapshot.BuildupMultiplier = FMath::Max(ActionState.BuildupMultiplier, 0.0f);

    bool bFoundBaseDamage = false;
    Snapshot.ResolvedBaseDamage = FallbackBaseDamage;
    if (BaseDamageAttribute.IsValid())
    {
        const float AttributeBaseDamage = GetAvatarAttributeValue(BaseDamageAttribute, bFoundBaseDamage);
        if (bFoundBaseDamage)
        {
            Snapshot.ResolvedBaseDamage = AttributeBaseDamage;
        }
    }

    bool bFoundStrength = false;
    Snapshot.StrengthValue = GetAvatarAttributeValue(UActionCombatStatsSet::GetStrengthAttribute(), bFoundStrength);
    if (!bFoundStrength)
    {
        Snapshot.StrengthValue = 0.0f;
    }

    bool bFoundDexterity = false;
    Snapshot.DexterityValue = GetAvatarAttributeValue(UActionCombatStatsSet::GetDexterityAttribute(), bFoundDexterity);
    if (!bFoundDexterity)
    {
        Snapshot.DexterityValue = 0.0f;
    }

    bool bFoundIntelligence = false;
    Snapshot.IntelligenceValue = GetAvatarAttributeValue(UActionCombatStatsSet::GetIntelligenceAttribute(), bFoundIntelligence);
    if (!bFoundIntelligence)
    {
        Snapshot.IntelligenceValue = 0.0f;
    }

    bool bFoundFaith = false;
    Snapshot.FaithValue = GetAvatarAttributeValue(UActionCombatStatsSet::GetFaithAttribute(), bFoundFaith);
    if (!bFoundFaith)
    {
        Snapshot.FaithValue = 0.0f;
    }

    bool bFoundArcane = false;
    Snapshot.ArcaneValue = GetAvatarAttributeValue(UActionCombatStatsSet::GetArcaneAttribute(), bFoundArcane);
    if (!bFoundArcane)
    {
        Snapshot.ArcaneValue = 0.0f;
    }

    bool bFoundCustomAttackPower = false;
    Snapshot.CustomAttackPowerValue = GetAvatarAttributeValue(UCustomStatusAttributeSet::GetAttackPowerAttribute(), bFoundCustomAttackPower);
    if (!bFoundCustomAttackPower)
    {
        Snapshot.CustomAttackPowerValue = 0.0f;
    }

    if (const AActor* AvatarActor = GetAvatarActorFromActorInfo())
    {
        if (const UActionCombatWeaponDefinition* WeaponDefinition = UActionCombatLyraEquipmentResolver::ResolveEquippedWeaponDefinition(AvatarActor, WeaponResolverData))
        {
            const int32 WeaponLevel = UActionCombatLyraEquipmentResolver::ResolveEquippedWeaponLevel(AvatarActor, WeaponResolverData);
            Snapshot.bUsesWeaponDefinition = true;
            Snapshot.ResolvedBaseDamage = WeaponDefinition->GetResolvedBaseDamage(WeaponLevel);
            Snapshot.StrengthScaling = WeaponDefinition->GetStrengthScaling();
            Snapshot.DexterityScaling = WeaponDefinition->GetDexterityScaling();
            Snapshot.IntelligenceScaling = WeaponDefinition->GetIntelligenceScaling();
            Snapshot.FaithScaling = WeaponDefinition->GetFaithScaling();
            Snapshot.ArcaneScaling = WeaponDefinition->GetArcaneScaling();
        }
    }

    return Snapshot;
}

TSubclassOf<UGameplayEffect> UActionCombatLyraGameplayAbility_MeleeEffect::ResolveDamageEffectClass() const
{
    if (bUseAttackSnapshotDamageExecution && SnapshotDamageEffectClass)
    {
        return SnapshotDamageEffectClass;
    }

    return TargetEffectClass;
}

void UActionCombatLyraGameplayAbility_MeleeEffect::ConfigureSnapshotDamageSpec(FGameplayEffectSpec& EffectSpec, const FActionCombatRecordedHit& RecordedHit) const
{
    const FActionCombatAttackSnapshot Snapshot = bHasActivationAttackSnapshot ? ActivationAttackSnapshot : MakeAttackSnapshot();

    EffectSpec.SetSetByCallerMagnitude(ActionCombatLyraBridgeTags::SetByCaller_Attack_BaseDamage, Snapshot.ResolvedBaseDamage);
    EffectSpec.SetSetByCallerMagnitude(ActionCombatLyraBridgeTags::SetByCaller_Attack_MotionValue, Snapshot.MotionValue);
    EffectSpec.SetSetByCallerMagnitude(ActionCombatLyraBridgeTags::SetByCaller_Attack_Strength, Snapshot.StrengthValue);
    EffectSpec.SetSetByCallerMagnitude(ActionCombatLyraBridgeTags::SetByCaller_Attack_Dexterity, Snapshot.DexterityValue);
    EffectSpec.SetSetByCallerMagnitude(ActionCombatLyraBridgeTags::SetByCaller_Attack_Intelligence, Snapshot.IntelligenceValue);
    EffectSpec.SetSetByCallerMagnitude(ActionCombatLyraBridgeTags::SetByCaller_Attack_Faith, Snapshot.FaithValue);
    EffectSpec.SetSetByCallerMagnitude(ActionCombatLyraBridgeTags::SetByCaller_Attack_Arcane, Snapshot.ArcaneValue);
    EffectSpec.SetSetByCallerMagnitude(ActionCombatLyraBridgeTags::SetByCaller_Attack_CustomAttackPower, Snapshot.CustomAttackPowerValue);
    EffectSpec.SetSetByCallerMagnitude(ActionCombatLyraBridgeTags::SetByCaller_Attack_StrengthScaling, Snapshot.StrengthScaling);
    EffectSpec.SetSetByCallerMagnitude(ActionCombatLyraBridgeTags::SetByCaller_Attack_DexterityScaling, Snapshot.DexterityScaling);
    EffectSpec.SetSetByCallerMagnitude(ActionCombatLyraBridgeTags::SetByCaller_Attack_IntelligenceScaling, Snapshot.IntelligenceScaling);
    EffectSpec.SetSetByCallerMagnitude(ActionCombatLyraBridgeTags::SetByCaller_Attack_FaithScaling, Snapshot.FaithScaling);
    EffectSpec.SetSetByCallerMagnitude(ActionCombatLyraBridgeTags::SetByCaller_Attack_ArcaneScaling, Snapshot.ArcaneScaling);
    EffectSpec.SetSetByCallerMagnitude(ActionCombatLyraBridgeTags::SetByCaller_Attack_PoiseDamage, Snapshot.PoiseDamage);
    EffectSpec.SetSetByCallerMagnitude(ActionCombatLyraBridgeTags::SetByCaller_Attack_BuildupMultiplier, Snapshot.BuildupMultiplier);

    if (DamageMultiplierSetByCallerTag.IsValid())
    {
        EffectSpec.SetSetByCallerMagnitude(DamageMultiplierSetByCallerTag, RecordedHit.DamageMultiplier);
    }

    if (bWriteFinalDamageToLyraSetByCallerDamage)
    {
        const float PreviewDamage = Snapshot.ComputePreviewDamage(RecordedHit.DamageMultiplier);
        EffectSpec.SetSetByCallerMagnitude(LyraGameplayTags::SetByCaller_Damage, PreviewDamage);

        UE_LOG(
            LogActionCombatRuntime,
            Log,
            TEXT("[MeleeAbility:%s] PreparedSnapshotDamage HitActor=%s BaseDamage=%.2f CustomAttackPower=%.2f StatScaling=%.2f MotionValue=%.2f Multiplier=%.2f FinalDamage=%.2f"),
            *GetPathNameSafe(GetAvatarActorFromActorInfo()),
            *GetNameSafe(RecordedHit.HitResult.GetActor()),
            Snapshot.ResolvedBaseDamage,
            Snapshot.CustomAttackPowerValue,
            Snapshot.ComputeStatScalingContribution(),
            Snapshot.MotionValue,
            RecordedHit.DamageMultiplier,
            PreviewDamage);
    }
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

void UActionCombatLyraGameplayAbility_MeleeEffect::TryApplyReactionToRecordedHit(AActor* HitActor, const FActionCombatRecordedHit& RecordedHit) const
{
    if (!bApplyReactionOnSuccessfulHit || (HitActor == nullptr))
    {
        return;
    }

    const AActor* AvatarActor = GetAvatarActorFromActorInfo();
    if (AvatarActor == nullptr)
    {
        return;
    }

    if (ULyraTeamSubsystem* TeamSubsystem = HitActor->GetWorld() ? HitActor->GetWorld()->GetSubsystem<ULyraTeamSubsystem>() : nullptr)
    {
        if (!TeamSubsystem->CanCauseDamage(AvatarActor, HitActor))
        {
            return;
        }
    }

    const FActionCombatAttackSnapshot Snapshot = bHasActivationAttackSnapshot ? ActivationAttackSnapshot : MakeAttackSnapshot();
    float ResolvedPoiseDamage = FMath::Max(Snapshot.PoiseDamage, 0.0f);
    if (bScaleReactionByRecordedHitMultiplier)
    {
        ResolvedPoiseDamage *= FMath::Max(RecordedHit.DamageMultiplier, 0.0f);
    }

    if (ResolvedPoiseDamage <= KINDA_SMALL_NUMBER)
    {
        return;
    }

    FActionCombatReactionResult ReactionResult;
    if (UActionCombatReactionComponent::ApplyReactionHitToActor(
        HitActor,
        const_cast<AActor*>(AvatarActor),
        ResolvedPoiseDamage,
        ResolvedPoiseDamage,
        FVector::ZeroVector,
        ReactionResult))
    {
        UE_LOG(
            LogActionCombatRuntime,
            Log,
            TEXT("[MeleeAbility:%s] ReactionResolved HitActor=%s Outcome=%d PoiseBefore=%.2f PoiseAfter=%.2f"),
            *GetPathNameSafe(GetAvatarActorFromActorInfo()),
            *GetNameSafe(HitActor),
            static_cast<int32>(ReactionResult.Outcome),
            ReactionResult.PoiseBefore,
            ReactionResult.PoiseAfter);
    }
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
        TryApplyReactionToRecordedHit(HitActor, RecordedHit);
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
