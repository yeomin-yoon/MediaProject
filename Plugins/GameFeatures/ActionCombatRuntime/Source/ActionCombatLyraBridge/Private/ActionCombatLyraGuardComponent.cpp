#include "ActionCombatLyraGuardComponent.h"

#include "ActionCombatComponent.h"
#include "ActionCombatLyraBridgeTags.h"
#include "ActionCombatRuntimeLog.h"
#include "ActionCombatStaminaSet.h"

#include "Abilities/GameplayAbilityTypes.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

UActionCombatLyraGuardComponent::UActionCombatLyraGuardComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    PrimaryComponentTick.bCanEverTick = false;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    SetIsReplicatedByDefault(true);
    GuardResourceAttribute = UActionCombatStaminaSet::GetStaminaAttribute();
}

void UActionCombatLyraGuardComponent::BeginPlay()
{
    Super::BeginPlay();

    if (HasGuardAuthority())
    {
        UpdateGuardGameplayTags();
    }
}

void UActionCombatLyraGuardComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    ClearAllTimers();
    Super::EndPlay(EndPlayReason);
}

void UActionCombatLyraGuardComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ThisClass, GuardState);
}

void UActionCombatLyraGuardComponent::SetGuardInputHeld(bool bNewGuardInputHeld)
{
    if (HasGuardAuthority())
    {
        SetGuardInputHeldAuthority(bNewGuardInputHeld);
        return;
    }

    ServerSetGuardInputHeld(bNewGuardInputHeld);
}

bool UActionCombatLyraGuardComponent::TryResolveIncomingHit(const FActionCombatLyraIncomingGuardHit& IncomingHit, FActionCombatLyraGuardResult& OutResult)
{
    OutResult = FActionCombatLyraGuardResult();

    if (!HasGuardAuthority())
    {
        LogGuard(TEXT("TryResolveIncomingHit ignored on non-authority."));
        return false;
    }

    if (!IncomingHit.bBlockable || !GuardState.bGuardActive || GuardState.bGuardBroken)
    {
        return false;
    }

    UAbilitySystemComponent* AbilitySystemComponent = ResolveAbilitySystemComponent();
    if (!AbilitySystemComponent || !GuardResourceAttribute.IsValid())
    {
        LogGuard(TEXT("TryResolveIncomingHit ignored because guard resource attribute or ASC was missing."));
        return false;
    }

    OutResult.FacingDot = ComputeFacingDot(IncomingHit);
    if (bRequireFacingForBlock && (OutResult.FacingDot < MinimumBlockDotProduct))
    {
        LogGuard(FString::Printf(TEXT("Incoming hit rejected by facing requirement. Dot=%.2f Required=%.2f"), OutResult.FacingDot, MinimumBlockDotProduct));
        return false;
    }

    OutResult.ResourceBefore = AbilitySystemComponent->GetNumericAttribute(GuardResourceAttribute);
    OutResult.AppliedGuardDamage = FMath::Max(IncomingHit.GuardDamage, 0.0f);
    if (OutResult.AppliedGuardDamage > 0.0f)
    {
        AbilitySystemComponent->ApplyModToAttribute(GuardResourceAttribute, EGameplayModOp::Additive, -OutResult.AppliedGuardDamage);
    }

    OutResult.ResourceAfter = AbilitySystemComponent->GetNumericAttribute(GuardResourceAttribute);
    OutResult.bDamagePrevented = true;

    if (OutResult.ResourceAfter <= KINDA_SMALL_NUMBER)
    {
        OutResult.Outcome = EActionCombatLyraGuardOutcome::GuardBroken;
        BreakGuardAuthority(ResolveGuardDuration(IncomingHit.GuardBreakDurationSeconds, DefaultGuardBreakDurationSeconds));
        DispatchGuardGameplayEvent(ActionCombatLyraBridgeTags::Combat_GameplayEvent_GuardBroken, IncomingHit, OutResult);
        OnGuardBroken.Broadcast();
        LogGuard(FString::Printf(TEXT("Guard broken. Resource %.2f -> %.2f"), OutResult.ResourceBefore, OutResult.ResourceAfter));
    }
    else
    {
        OutResult.Outcome = EActionCombatLyraGuardOutcome::Blocked;
        StartOrRefreshForcedGuardAuthority(ResolveGuardDuration(IncomingHit.ForcedGuardDurationSeconds, DefaultForcedGuardDurationSeconds));
        DispatchGuardGameplayEvent(ActionCombatLyraBridgeTags::Combat_GameplayEvent_GuardBlocked, IncomingHit, OutResult);
        LogGuard(FString::Printf(TEXT("Blocked incoming hit. Resource %.2f -> %.2f"), OutResult.ResourceBefore, OutResult.ResourceAfter));
    }

    OnGuardBlocked.Broadcast(OutResult.Outcome, OutResult.AppliedGuardDamage);
    return true;
}

void UActionCombatLyraGuardComponent::ForceGuardBreak(float RecoverySeconds)
{
    if (!HasGuardAuthority())
    {
        return;
    }

    BreakGuardAuthority(ResolveGuardDuration(RecoverySeconds, DefaultGuardBreakDurationSeconds));
}

void UActionCombatLyraGuardComponent::RefreshGuardStateTags()
{
    if (!HasGuardAuthority())
    {
        return;
    }

    UpdateGuardGameplayTags();
}

UActionCombatLyraGuardComponent* UActionCombatLyraGuardComponent::FindGuardComponent(const AActor* Actor)
{
    return Actor ? Actor->FindComponentByClass<UActionCombatLyraGuardComponent>() : nullptr;
}

void UActionCombatLyraGuardComponent::ServerSetGuardInputHeld_Implementation(bool bNewGuardInputHeld)
{
    SetGuardInputHeldAuthority(bNewGuardInputHeld);
}

void UActionCombatLyraGuardComponent::OnRep_GuardState(FActionCombatLyraReplicatedGuardState PreviousState)
{
    BroadcastGuardStateChange(PreviousState);
}

void UActionCombatLyraGuardComponent::SetGuardInputHeldAuthority(bool bNewGuardInputHeld)
{
    if (GuardState.bGuardBroken && bNewGuardInputHeld)
    {
        LogGuard(TEXT("Guard input ignored because guard is broken."));
        return;
    }

    if (bBlockGuardStartDuringCombatAction && bNewGuardInputHeld && !GuardState.bGuardInputHeld && IsCombatActionActive())
    {
        LogGuard(TEXT("Guard input ignored because a combat action is active."));
        return;
    }

    if (GuardState.bGuardInputHeld == bNewGuardInputHeld)
    {
        return;
    }

    const FActionCombatLyraReplicatedGuardState PreviousState = GuardState;
    GuardState.bGuardInputHeld = bNewGuardInputHeld;

    if (bNewGuardInputHeld)
    {
        if (!GuardState.bGuardActive)
        {
            if (GuardActivationDelaySeconds <= 0.0f)
            {
                GuardState.bGuardActive = true;
            }
            else if (GetWorld())
            {
                GetWorld()->GetTimerManager().SetTimer(GuardActivationTimerHandle, this, &ThisClass::HandleGuardActivationTimerExpired, GuardActivationDelaySeconds, false);
            }
        }
    }
    else
    {
        if (GetWorld())
        {
            GetWorld()->GetTimerManager().ClearTimer(GuardActivationTimerHandle);
        }

        if (!GuardState.bForcedGuardActive)
        {
            GuardState.bGuardActive = false;
        }
    }

    HandleGuardStateMutated(PreviousState);
    LogGuard(FString::Printf(TEXT("GuardInputHeld=%s"), GuardState.bGuardInputHeld ? TEXT("true") : TEXT("false")));
}

void UActionCombatLyraGuardComponent::SetGuardActiveAuthority(bool bNewGuardActive)
{
    if (GuardState.bGuardActive == bNewGuardActive)
    {
        return;
    }

    const FActionCombatLyraReplicatedGuardState PreviousState = GuardState;
    GuardState.bGuardActive = bNewGuardActive;
    HandleGuardStateMutated(PreviousState);
}

void UActionCombatLyraGuardComponent::StartOrRefreshForcedGuardAuthority(float DurationSeconds)
{
    const FActionCombatLyraReplicatedGuardState PreviousState = GuardState;
    GuardState.bGuardActive = true;
    GuardState.bForcedGuardActive = DurationSeconds > 0.0f;

    if (GetWorld())
    {
        if (DurationSeconds > 0.0f)
        {
            GetWorld()->GetTimerManager().SetTimer(ForcedGuardTimerHandle, this, &ThisClass::HandleForcedGuardTimerExpired, DurationSeconds, false);
        }
        else
        {
            GetWorld()->GetTimerManager().ClearTimer(ForcedGuardTimerHandle);
        }
    }

    HandleGuardStateMutated(PreviousState);
}

void UActionCombatLyraGuardComponent::BreakGuardAuthority(float RecoverySeconds)
{
    const FActionCombatLyraReplicatedGuardState PreviousState = GuardState;

    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(GuardActivationTimerHandle);
        GetWorld()->GetTimerManager().ClearTimer(ForcedGuardTimerHandle);
    }

    GuardState.bGuardInputHeld = false;
    GuardState.bGuardActive = false;
    GuardState.bForcedGuardActive = false;
    GuardState.bGuardBroken = true;

    if (GetWorld())
    {
        GetWorld()->GetTimerManager().SetTimer(GuardBrokenTimerHandle, this, &ThisClass::HandleGuardBrokenTimerExpired, FMath::Max(RecoverySeconds, 0.01f), false);
    }

    HandleGuardStateMutated(PreviousState);
}

void UActionCombatLyraGuardComponent::ClearAllTimers()
{
    if (!GetWorld())
    {
        return;
    }

    GetWorld()->GetTimerManager().ClearTimer(GuardActivationTimerHandle);
    GetWorld()->GetTimerManager().ClearTimer(ForcedGuardTimerHandle);
    GetWorld()->GetTimerManager().ClearTimer(GuardBrokenTimerHandle);
}

void UActionCombatLyraGuardComponent::HandleGuardActivationTimerExpired()
{
    if (!HasGuardAuthority() || GuardState.bGuardBroken || !GuardState.bGuardInputHeld)
    {
        return;
    }

    SetGuardActiveAuthority(true);
    LogGuard(TEXT("Guard active after startup delay."));
}

void UActionCombatLyraGuardComponent::HandleForcedGuardTimerExpired()
{
    if (!HasGuardAuthority())
    {
        return;
    }

    const FActionCombatLyraReplicatedGuardState PreviousState = GuardState;
    GuardState.bForcedGuardActive = false;
    if (!GuardState.bGuardInputHeld)
    {
        GuardState.bGuardActive = false;
    }

    HandleGuardStateMutated(PreviousState);
    LogGuard(TEXT("Forced guard window ended."));
}

void UActionCombatLyraGuardComponent::HandleGuardBrokenTimerExpired()
{
    if (!HasGuardAuthority())
    {
        return;
    }

    const FActionCombatLyraReplicatedGuardState PreviousState = GuardState;
    GuardState.bGuardBroken = false;
    HandleGuardStateMutated(PreviousState);
    LogGuard(TEXT("Guard break recovery finished."));
}

void UActionCombatLyraGuardComponent::HandleGuardStateMutated(const FActionCombatLyraReplicatedGuardState& PreviousState)
{
    if (PreviousState.Equals(GuardState))
    {
        return;
    }

    if (HasGuardAuthority())
    {
        UpdateGuardGameplayTags();
    }

    if (!PreviousState.bGuardActive && GuardState.bGuardActive && bInterruptCombatActionsOnGuardStart)
    {
        InterruptCombatActionIfNeeded();
    }

    if (!PreviousState.bGuardBroken && GuardState.bGuardBroken && bInterruptCombatActionsOnGuardBreak)
    {
        InterruptCombatActionIfNeeded();
    }

    BroadcastGuardStateChange(PreviousState);
}

void UActionCombatLyraGuardComponent::BroadcastGuardStateChange(const FActionCombatLyraReplicatedGuardState& PreviousState)
{
    if (PreviousState.Equals(GuardState))
    {
        return;
    }

    OnGuardStateChanged.Broadcast(GuardState);
}

void UActionCombatLyraGuardComponent::UpdateGuardGameplayTags()
{
    UAbilitySystemComponent* AbilitySystemComponent = ResolveAbilitySystemComponent();
    if (!AbilitySystemComponent)
    {
        return;
    }

    AbilitySystemComponent->SetLooseGameplayTagCount(ActionCombatLyraBridgeTags::Combat_State_Guard, GuardState.bGuardActive ? 1 : 0);
    AbilitySystemComponent->SetLooseGameplayTagCount(ActionCombatLyraBridgeTags::Combat_State_ForcedGuard, GuardState.bForcedGuardActive ? 1 : 0);
    AbilitySystemComponent->SetLooseGameplayTagCount(ActionCombatLyraBridgeTags::Combat_State_GuardBroken, GuardState.bGuardBroken ? 1 : 0);
}

void UActionCombatLyraGuardComponent::DispatchGuardGameplayEvent(const FGameplayTag& EventTag, const FActionCombatLyraIncomingGuardHit& IncomingHit, const FActionCombatLyraGuardResult& GuardResult)
{
    if (!bDispatchGameplayEvents || !EventTag.IsValid())
    {
        return;
    }

    UAbilitySystemComponent* AbilitySystemComponent = ResolveAbilitySystemComponent();
    if (!AbilitySystemComponent)
    {
        return;
    }

    FGameplayEventData Payload;
    Payload.EventTag = EventTag;
    Payload.Instigator = IncomingHit.Attacker;
    Payload.Target = GetOwner();
    Payload.OptionalObject = this;
    Payload.EventMagnitude = GuardResult.AppliedGuardDamage;
    Payload.ContextHandle = AbilitySystemComponent->MakeEffectContext();
    Payload.ContextHandle.AddHitResult(IncomingHit.HitResult, true);

    AbilitySystemComponent->HandleGameplayEvent(EventTag, &Payload);
}

void UActionCombatLyraGuardComponent::InterruptCombatActionIfNeeded()
{
    if (UActionCombatComponent* CombatComponent = ResolveActionCombatComponent())
    {
        CombatComponent->InterruptActiveAction();
    }
}

bool UActionCombatLyraGuardComponent::IsCombatActionActive() const
{
    if (const UActionCombatComponent* CombatComponent = ResolveActionCombatComponent())
    {
        if (CombatComponent->GetActiveActionState().ActionTag.IsValid())
        {
            return true;
        }
    }

    const UAbilitySystemComponent* AbilitySystemComponent = ResolveAbilitySystemComponent();
    return AbilitySystemComponent
        && AbilitySystemComponent->HasMatchingGameplayTag(ActionCombatLyraBridgeTags::Combat_State_Action);
}

bool UActionCombatLyraGuardComponent::HasGuardAuthority() const
{
    const AActor* Owner = GetOwner();
    return Owner != nullptr && Owner->HasAuthority();
}

float UActionCombatLyraGuardComponent::ResolveGuardDuration(float RequestedDurationSeconds, float DefaultDurationSeconds) const
{
    return RequestedDurationSeconds > 0.0f ? RequestedDurationSeconds : DefaultDurationSeconds;
}

float UActionCombatLyraGuardComponent::ComputeFacingDot(const FActionCombatLyraIncomingGuardHit& IncomingHit) const
{
    const AActor* Owner = GetOwner();
    if (!Owner)
    {
        return -1.0f;
    }

    FVector ToAttacker = FVector::ZeroVector;
    if (IncomingHit.Attacker)
    {
        ToAttacker = IncomingHit.Attacker->GetActorLocation() - Owner->GetActorLocation();
    }
    else if (IncomingHit.HitResult.ImpactPoint != FVector::ZeroVector)
    {
        ToAttacker = IncomingHit.HitResult.ImpactPoint - Owner->GetActorLocation();
    }

    const FVector Forward2D = Owner->GetActorForwardVector().GetSafeNormal2D();
    const FVector ToAttacker2D = ToAttacker.GetSafeNormal2D();
    if (Forward2D.IsNearlyZero() || ToAttacker2D.IsNearlyZero())
    {
        return 1.0f;
    }

    return FVector::DotProduct(Forward2D, ToAttacker2D);
}

UAbilitySystemComponent* UActionCombatLyraGuardComponent::ResolveAbilitySystemComponent() const
{
    return UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner());
}

UActionCombatComponent* UActionCombatLyraGuardComponent::ResolveActionCombatComponent() const
{
    if (AActor* Owner = GetOwner())
    {
        if (UActionCombatComponent* ExplicitComponent = Cast<UActionCombatComponent>(ActionCombatComponentReference.GetComponent(Owner)))
        {
            return ExplicitComponent;
        }

        return Owner->FindComponentByClass<UActionCombatComponent>();
    }

    return nullptr;
}

void UActionCombatLyraGuardComponent::LogGuard(const FString& Message) const
{
    if (!bLogGuardFlow)
    {
        return;
    }

    UE_LOG(LogActionCombatRuntime, Log, TEXT("[Guard:%s] %s"), *GetPathNameSafe(GetOwner()), *Message);
}
