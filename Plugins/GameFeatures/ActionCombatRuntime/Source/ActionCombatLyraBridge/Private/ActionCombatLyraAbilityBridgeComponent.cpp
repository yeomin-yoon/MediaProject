#include "ActionCombatLyraAbilityBridgeComponent.h"

#include "ActionCombatBlueprintLibrary.h"
#include "ActionCombatLyraBridgeTags.h"
#include "ActionCombatComponent.h"
#include "ActionCombatMeleeTraceComponent.h"
#include "ActionCombatRuntimeLog.h"
#include "ActionCombatStaminaSet.h"
#include "LockOnComponent.h"

#include "Abilities/GameplayAbility.h"
#include "AbilitySystem/LyraAbilitySystemComponent.h"
#include "Character/LyraPawnExtensionComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"

UActionCombatLyraAbilityBridgeComponent::UActionCombatLyraAbilityBridgeComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    ObservedDashAbilityTag = ActionCombatLyraBridgeTags::Ability_Type_Action_Dash;
    GrantedDashStateTag = ActionCombatLyraBridgeTags::Combat_State_Dodge;
    GrantedDashIFrameStateTag = ActionCombatLyraBridgeTags::Combat_State_Dodge_IFrame;
    FallbackDashAbilityClass = TSoftClassPtr<UGameplayAbility>(FSoftClassPath(TEXT("/ShooterCore/Game/Dash/GA_Hero_Dash.GA_Hero_Dash_C")));
    FallbackDashInputTag = FGameplayTag::RequestGameplayTag(TEXT("InputTag.Ability.Dash"), false);
    DefaultActionStartedEventTag = FGameplayTag::RequestGameplayTag(TEXT("Combat.GameplayEvent.Action.Started"), false);
    DefaultActionEndedEventTag = FGameplayTag::RequestGameplayTag(TEXT("Combat.GameplayEvent.Action.Ended"), false);
}

void UActionCombatLyraAbilityBridgeComponent::BeginPlay()
{
    Super::BeginPlay();
    BindPawnExtension();
    TryBindCombatComponent();
    HandleAbilitySystemInitialized();
}

void UActionCombatLyraAbilityBridgeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    UnbindLyraAbilitySystemComponent();
    UnbindCombatComponent();
    Super::EndPlay(EndPlayReason);
}

void UActionCombatLyraAbilityBridgeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    RegenerateStamina(DeltaTime);
    TickActionFacing(DeltaTime);
}

void UActionCombatLyraAbilityBridgeComponent::RefreshCombatBinding()
{
    UnbindCombatComponent();
    TryBindCombatComponent();
    HandleAbilitySystemInitialized();
}

void UActionCombatLyraAbilityBridgeComponent::BindPawnExtension()
{
    if (ULyraPawnExtensionComponent* PawnExtension = ResolvePawnExtensionComponent())
    {
        PawnExtension->OnAbilitySystemInitialized_RegisterAndCall(FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::HandleAbilitySystemInitialized));
        PawnExtension->OnAbilitySystemUninitialized_Register(FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::HandleAbilitySystemUninitialized));
    }
}

void UActionCombatLyraAbilityBridgeComponent::TryBindCombatComponent()
{
    UActionCombatComponent* CombatComponent = ResolveActionCombatComponent();
    if (!CombatComponent)
    {
        return;
    }

    CombatComponent->OnActionStarted.AddDynamic(this, &ThisClass::HandleActionStarted);
    CombatComponent->OnActionEnded.AddDynamic(this, &ThisClass::HandleActionEnded);
    BoundCombatComponent = CombatComponent;
    LogBridge(TEXT("Bound to ActionCombatComponent."));
}

void UActionCombatLyraAbilityBridgeComponent::UnbindCombatComponent()
{
    if (UActionCombatComponent* CombatComponent = BoundCombatComponent.Get())
    {
        CombatComponent->OnActionStarted.RemoveDynamic(this, &ThisClass::HandleActionStarted);
        CombatComponent->OnActionEnded.RemoveDynamic(this, &ThisClass::HandleActionEnded);
    }

    BoundCombatComponent.Reset();
}

void UActionCombatLyraAbilityBridgeComponent::BindLyraAbilitySystemComponent(ULyraAbilitySystemComponent* AbilitySystemComponent)
{
    if (BoundAbilitySystemComponent.Get() == AbilitySystemComponent)
    {
        RefreshMirroredDashStateFromAbilitySystem();
        return;
    }

    UnbindLyraAbilitySystemComponent();

    if (!AbilitySystemComponent)
    {
        return;
    }

    BoundAbilitySystemComponent = AbilitySystemComponent;
    bObservedDashAbilityTagActive = false;
    ActiveObservedDashAbilityCount = 0;
    EnsureStaminaAttributeSetRegistered();
    EnsureFallbackDashAbilityGranted();

    const APawn* Pawn = ResolvePawnOwner();
    const bool bShouldMirrorDashState = bMirrorDashAbilityToCombatState
        && ObservedDashAbilityTag.IsValid()
        && (!bMirrorDashStateOnlyOnAuthority || (Pawn && Pawn->HasAuthority()));

    if (!bShouldMirrorDashState)
    {
        return;
    }

    DashAbilityTagChangedHandle = AbilitySystemComponent->RegisterGameplayTagEvent(ObservedDashAbilityTag).AddUObject(this, &ThisClass::HandleObservedDashAbilityTagChanged);
    DashAbilityActivatedHandle = AbilitySystemComponent->AbilityActivatedCallbacks.AddUObject(this, &ThisClass::HandleObservedAbilityActivated);
    DashAbilityEndedHandle = AbilitySystemComponent->AbilityEndedCallbacks.AddUObject(this, &ThisClass::HandleObservedAbilityEnded);
    RefreshMirroredDashStateFromAbilitySystem();
    LogBridge(FString::Printf(TEXT("Bound dash dodge mirroring to ASC. ObservedTag=%s"), *ObservedDashAbilityTag.ToString()));
}

void UActionCombatLyraAbilityBridgeComponent::UnbindLyraAbilitySystemComponent()
{
    if (ULyraAbilitySystemComponent* AbilitySystemComponent = BoundAbilitySystemComponent.Get())
    {
        if (DashAbilityTagChangedHandle.IsValid() && ObservedDashAbilityTag.IsValid())
        {
            AbilitySystemComponent->UnregisterGameplayTagEvent(DashAbilityTagChangedHandle, ObservedDashAbilityTag);
            DashAbilityTagChangedHandle.Reset();
        }

        if (DashAbilityActivatedHandle.IsValid())
        {
            AbilitySystemComponent->AbilityActivatedCallbacks.Remove(DashAbilityActivatedHandle);
            DashAbilityActivatedHandle.Reset();
        }

        if (DashAbilityEndedHandle.IsValid())
        {
            AbilitySystemComponent->AbilityEndedCallbacks.Remove(DashAbilityEndedHandle);
            DashAbilityEndedHandle.Reset();
        }

        SetMirroredDashStateActive(false);
    }

    BoundAbilitySystemComponent.Reset();
    OwnedStaminaSet = nullptr;
    SetActionMovementBlockActive(false);
    ClearActionFacing();
    bMirroredDashStateActive = false;
    bObservedDashAbilityTagActive = false;
    ActiveObservedDashAbilityCount = 0;
    RefreshComponentTickEnabled();
}

void UActionCombatLyraAbilityBridgeComponent::RefreshComponentTickEnabled()
{
    SetComponentTickEnabled(ShouldTickForStaminaRegen() || bActionFacingActive);
}

bool UActionCombatLyraAbilityBridgeComponent::ShouldTickForStaminaRegen() const
{
    const APawn* Pawn = ResolvePawnOwner();
    return bRegenerateStaminaOnAuthority && Pawn && Pawn->HasAuthority() && BoundAbilitySystemComponent.IsValid();
}

void UActionCombatLyraAbilityBridgeComponent::EnsureStaminaAttributeSetRegistered()
{
    if (!bEnsureStaminaAttributeSet)
    {
        return;
    }

    ULyraAbilitySystemComponent* AbilitySystemComponent = BoundAbilitySystemComponent.Get();
    if (!AbilitySystemComponent)
    {
        return;
    }

    if (!AbilitySystemComponent->HasAttributeSetForAttribute(UActionCombatStaminaSet::GetStaminaAttribute()))
    {
        OwnedStaminaSet = NewObject<UActionCombatStaminaSet>(AbilitySystemComponent->GetOwner());
        AbilitySystemComponent->AddAttributeSetSubobject(OwnedStaminaSet.Get());
        LogBridge(TEXT("Added ActionCombat stamina attribute set to ASC."));
    }
    else
    {
        OwnedStaminaSet = const_cast<UActionCombatStaminaSet*>(AbilitySystemComponent->GetSet<UActionCombatStaminaSet>());
    }

    LastObservedStamina = AbilitySystemComponent->GetNumericAttribute(UActionCombatStaminaSet::GetStaminaAttribute());
    LastStaminaSpendTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : -1000.0f;
    RefreshComponentTickEnabled();
}

void UActionCombatLyraAbilityBridgeComponent::RegenerateStamina(float DeltaTime)
{
    if (!bRegenerateStaminaOnAuthority || DeltaTime <= 0.0f)
    {
        return;
    }

    APawn* Pawn = ResolvePawnOwner();
    ULyraAbilitySystemComponent* AbilitySystemComponent = BoundAbilitySystemComponent.Get();
    UWorld* World = GetWorld();
    if (!Pawn || !Pawn->HasAuthority() || !AbilitySystemComponent || !World)
    {
        return;
    }

    if (!AbilitySystemComponent->HasAttributeSetForAttribute(UActionCombatStaminaSet::GetStaminaAttribute()))
    {
        EnsureStaminaAttributeSetRegistered();
        if (!AbilitySystemComponent->HasAttributeSetForAttribute(UActionCombatStaminaSet::GetStaminaAttribute()))
        {
            return;
        }
    }

    const float CurrentStamina = AbilitySystemComponent->GetNumericAttribute(UActionCombatStaminaSet::GetStaminaAttribute());
    const float MaxStamina = FMath::Max(AbilitySystemComponent->GetNumericAttribute(UActionCombatStaminaSet::GetMaxStaminaAttribute()), 1.0f);
    const float RegenRate = FMath::Max(AbilitySystemComponent->GetNumericAttribute(UActionCombatStaminaSet::GetStaminaRegenRateAttribute()), 0.0f);

    if (CurrentStamina + KINDA_SMALL_NUMBER < LastObservedStamina)
    {
        LastStaminaSpendTimeSeconds = World->GetTimeSeconds();
    }

    LastObservedStamina = CurrentStamina;

    if (CurrentStamina >= MaxStamina || RegenRate <= 0.0f)
    {
        return;
    }

    if ((World->GetTimeSeconds() - LastStaminaSpendTimeSeconds) < StaminaRegenDelayAfterSpendSeconds)
    {
        return;
    }

    const float NewStamina = FMath::Min(CurrentStamina + RegenRate * DeltaTime, MaxStamina);
    if (NewStamina > CurrentStamina)
    {
        AbilitySystemComponent->ApplyModToAttribute(UActionCombatStaminaSet::GetStaminaAttribute(), EGameplayModOp::Override, NewStamina);
        LastObservedStamina = NewStamina;
    }
}

void UActionCombatLyraAbilityBridgeComponent::TryStartActionFacing()
{
    if (!bFaceLockOnTargetOnActionStart)
    {
        return;
    }

    APawn* Pawn = ResolvePawnOwner();
    ULockOnComponent* LockOnComponent = ResolveLockOnComponent();
    if (!Pawn || !LockOnComponent || !LockOnComponent->IsLockActive())
    {
        ClearActionFacing();
        return;
    }

    FVector FocusLocation = FVector::ZeroVector;
    if (!LockOnComponent->GetCurrentTargetFocusLocation(FocusLocation))
    {
        ClearActionFacing();
        return;
    }

    const FVector ToTarget2D = (FocusLocation - Pawn->GetActorLocation()).GetSafeNormal2D();
    if (ToTarget2D.IsNearlyZero())
    {
        ClearActionFacing();
        return;
    }

    const AController* Controller = Pawn->GetController();
    const float CurrentYaw = Controller ? Controller->GetControlRotation().Yaw : Pawn->GetActorRotation().Yaw;
    const float TargetYaw = ToTarget2D.Rotation().Yaw;
    const float DeltaYaw = FMath::Abs(FMath::FindDeltaAngleDegrees(CurrentYaw, TargetYaw));
    if ((MaxActionFacingYawDegrees > 0.0f) && (DeltaYaw > MaxActionFacingYawDegrees))
    {
        ClearActionFacing();
        LogBridge(FString::Printf(TEXT("Skipped action facing because yaw delta %.1f exceeded max %.1f."), DeltaYaw, MaxActionFacingYawDegrees));
        return;
    }

    if (ActionFacingTurnDurationSeconds <= KINDA_SMALL_NUMBER)
    {
        ClearActionFacing();
        ApplyFacingYaw(TargetYaw);
        return;
    }

    bActionFacingActive = true;
    ActionFacingStartYaw = CurrentYaw;
    ActionFacingTargetYaw = TargetYaw;
    ActionFacingElapsedSeconds = 0.0f;
    ActiveActionFacingDurationSeconds = ActionFacingTurnDurationSeconds;
    RefreshComponentTickEnabled();
}

void UActionCombatLyraAbilityBridgeComponent::TickActionFacing(float DeltaTime)
{
    if (!bActionFacingActive)
    {
        return;
    }

    if (DeltaTime <= 0.0f)
    {
        return;
    }

    ActionFacingElapsedSeconds += DeltaTime;
    const float SafeDuration = FMath::Max(ActiveActionFacingDurationSeconds, KINDA_SMALL_NUMBER);
    const float Alpha = FMath::Clamp(ActionFacingElapsedSeconds / SafeDuration, 0.0f, 1.0f);
    const float InterpolatedYaw = ActionFacingStartYaw
        + (FMath::FindDeltaAngleDegrees(ActionFacingStartYaw, ActionFacingTargetYaw) * Alpha);

    ApplyFacingYaw(InterpolatedYaw);

    if (Alpha >= 1.0f)
    {
        ClearActionFacing();
    }
}

void UActionCombatLyraAbilityBridgeComponent::ClearActionFacing()
{
    bActionFacingActive = false;
    ActionFacingElapsedSeconds = 0.0f;
    ActiveActionFacingDurationSeconds = 0.0f;
    RefreshComponentTickEnabled();
}

void UActionCombatLyraAbilityBridgeComponent::ApplyFacingYaw(float NewYaw) const
{
    APawn* Pawn = ResolvePawnOwner();
    if (!Pawn)
    {
        return;
    }

    const FRotator DesiredRotation(0.0f, NewYaw, 0.0f);
    if (AController* Controller = Pawn->GetController())
    {
        FRotator ControlRotation = Controller->GetControlRotation();
        ControlRotation.Yaw = NewYaw;
        Controller->SetControlRotation(ControlRotation);
    }

    Pawn->FaceRotation(DesiredRotation, 0.0f);
}

ULockOnComponent* UActionCombatLyraAbilityBridgeComponent::ResolveLockOnComponent() const
{
    if (AActor* Owner = GetOwner())
    {
        return Owner->FindComponentByClass<ULockOnComponent>();
    }

    return nullptr;
}

void UActionCombatLyraAbilityBridgeComponent::EnsureFallbackDashAbilityGranted()
{
    if (!bGrantFallbackDashAbilityOnServerIfMissing)
    {
        return;
    }

    APawn* Pawn = ResolvePawnOwner();
    ULyraAbilitySystemComponent* AbilitySystemComponent = BoundAbilitySystemComponent.Get();
    if (!Pawn || !Pawn->HasAuthority() || !AbilitySystemComponent)
    {
        return;
    }

    if (HasObservedDashAbilitySpec(*AbilitySystemComponent))
    {
        return;
    }

    TSubclassOf<UGameplayAbility> DashAbilityClass = FallbackDashAbilityClass.LoadSynchronous();
    if (!DashAbilityClass)
    {
        LogBridge(TEXT("Fallback dash ability grant skipped because the configured class could not be loaded."));
        return;
    }

    FGameplayAbilitySpec DashAbilitySpec(DashAbilityClass, 1, INDEX_NONE, this);
    if (FallbackDashInputTag.IsValid())
    {
        DashAbilitySpec.GetDynamicSpecSourceTags().AddTag(FallbackDashInputTag);
    }

    AbilitySystemComponent->GiveAbility(DashAbilitySpec);
    LogBridge(FString::Printf(TEXT("Granted fallback dash ability %s with input tag %s."), *GetNameSafe(DashAbilityClass), *FallbackDashInputTag.ToString()));
}

void UActionCombatLyraAbilityBridgeComponent::SetActionMovementBlockActive(bool bNewActive)
{
    ULyraAbilitySystemComponent* AbilitySystemComponent = BoundAbilitySystemComponent.Get();
    if (!AbilitySystemComponent)
    {
        AbilitySystemComponent = ResolveLyraAbilitySystemComponent();
    }

    if (!AbilitySystemComponent)
    {
        return;
    }

    AbilitySystemComponent->SetLooseGameplayTagCount(ActionCombatLyraBridgeTags::Combat_State_Action, bNewActive ? 1 : 0);
}

bool UActionCombatLyraAbilityBridgeComponent::HasObservedDashAbilitySpec(const ULyraAbilitySystemComponent& AbilitySystemComponent) const
{
    for (const FGameplayAbilitySpec& AbilitySpec : AbilitySystemComponent.GetActivatableAbilities())
    {
        if (IsObservedDashAbility(AbilitySpec.Ability) || AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(FallbackDashInputTag))
        {
            return true;
        }
    }

    return false;
}

void UActionCombatLyraAbilityBridgeComponent::RefreshMirroredDashStateFromAbilitySystem()
{
    const ULyraAbilitySystemComponent* AbilitySystemComponent = BoundAbilitySystemComponent.Get();
    bObservedDashAbilityTagActive = AbilitySystemComponent && ObservedDashAbilityTag.IsValid() && AbilitySystemComponent->HasMatchingGameplayTag(ObservedDashAbilityTag);
    const bool bDashAbilityActive = bObservedDashAbilityTagActive || (ActiveObservedDashAbilityCount > 0);
    SetMirroredDashStateActive(bDashAbilityActive);
}

void UActionCombatLyraAbilityBridgeComponent::SetMirroredDashStateActive(bool bNewDashStateActive)
{
    if (!bMirrorDashAbilityToCombatState)
    {
        return;
    }

    ULyraAbilitySystemComponent* AbilitySystemComponent = BoundAbilitySystemComponent.Get();
    if (!AbilitySystemComponent)
    {
        bMirroredDashStateActive = false;
        return;
    }

    if (bMirroredDashStateActive == bNewDashStateActive)
    {
        return;
    }

    bMirroredDashStateActive = bNewDashStateActive;

    if (GrantedDashStateTag.IsValid())
    {
        AbilitySystemComponent->SetLooseGameplayTagCount(GrantedDashStateTag, bMirroredDashStateActive ? 1 : 0);
    }

    if (GrantedDashIFrameStateTag.IsValid())
    {
        const int32 NewIFrameCount = (bMirroredDashStateActive && bMirrorDashAbilityToIFrameState) ? 1 : 0;
        AbilitySystemComponent->SetLooseGameplayTagCount(GrantedDashIFrameStateTag, NewIFrameCount);
    }

    if (bMirroredDashStateActive && bInterruptActiveCombatActionOnDashStart)
    {
        if (UActionCombatComponent* CombatComponent = ResolveActionCombatComponent())
        {
            CombatComponent->InterruptActiveAction();
        }
    }

    LogBridge(FString::Printf(
        TEXT("Dash dodge state %s. DodgeTag=%s IFrameTag=%s"),
        bMirroredDashStateActive ? TEXT("Enabled") : TEXT("Disabled"),
        *GrantedDashStateTag.ToString(),
        *GrantedDashIFrameStateTag.ToString()));
}

bool UActionCombatLyraAbilityBridgeComponent::IsObservedDashAbility(const UGameplayAbility* Ability) const
{
    if (!Ability || !ObservedDashAbilityTag.IsValid())
    {
        return false;
    }

    return Ability->GetAssetTags().HasTagExact(ObservedDashAbilityTag)
        || Ability->GetName().Contains(TEXT("Dash"));
}

void UActionCombatLyraAbilityBridgeComponent::HandleActionStarted(FActionCombatActiveActionState ActionState)
{
    SetActionMovementBlockActive(true);
    TryStartActionFacing();

    if (!bDispatchActionStartedEvents)
    {
        return;
    }

    const FGameplayTag EventTag = ResolveStartedEventTag(ActionState.ActionTag);
    DispatchEventForActionState(EventTag, ActionState, false);
}

void UActionCombatLyraAbilityBridgeComponent::HandleActionEnded(FActionCombatActiveActionState ActionState)
{
    SetActionMovementBlockActive(false);
    ClearActionFacing();

    if (!bDispatchActionEndedEvents)
    {
        return;
    }

    const FGameplayTag EventTag = ResolveEndedEventTag(ActionState.ActionTag);
    DispatchEventForActionState(EventTag, ActionState, true);
}

void UActionCombatLyraAbilityBridgeComponent::DispatchEventForActionState(const FGameplayTag& EventTag, const FActionCombatActiveActionState& ActionState, bool bIsEndEvent)
{
    if (!EventTag.IsValid())
    {
        LogBridge(FString::Printf(TEXT("Gameplay event skipped because %s event tag was invalid for action %s instance %d."), bIsEndEvent ? TEXT("ended") : TEXT("started"), *ActionState.ActionTag.ToString(), ActionState.ActionInstanceId));
        return;
    }

    APawn* Pawn = ResolvePawnOwner();
    if (!Pawn)
    {
        return;
    }

    if (bDispatchEventsOnlyOnAuthority && !Pawn->HasAuthority())
    {
        return;
    }

    ULyraAbilitySystemComponent* AbilitySystemComponent = ResolveLyraAbilitySystemComponent();
    if (!AbilitySystemComponent)
    {
        LogBridge(FString::Printf(TEXT("Gameplay event skipped because ASC was missing. EventTag=%s"), *EventTag.ToString()));
        return;
    }

    FGameplayEventData Payload;
    Payload.EventTag = EventTag;
    Payload.Instigator = Pawn;
    Payload.Target = Pawn;
    Payload.OptionalObject = BoundCombatComponent.Get();
    Payload.OptionalObject2 = UActionCombatBlueprintLibrary::FindMeleeTraceComponent(Pawn, ActionState.TraceSourceId, true);
    Payload.EventMagnitude = static_cast<float>(ActionState.ActionInstanceId);
    Payload.ContextHandle = AbilitySystemComponent->MakeEffectContext();
    Payload.InstigatorTags.AddTag(ActionState.ActionTag);

    AbilitySystemComponent->HandleGameplayEvent(EventTag, &Payload);
    LogBridge(FString::Printf(TEXT("Dispatched %s event %s for action %s instance %d."), bIsEndEvent ? TEXT("ended") : TEXT("started"), *EventTag.ToString(), *ActionState.ActionTag.ToString(), ActionState.ActionInstanceId));
}

void UActionCombatLyraAbilityBridgeComponent::LogBridge(const FString& Message) const
{
    if (!bLogAbilityBridge)
    {
        return;
    }

    UE_LOG(LogActionCombatRuntime, Log, TEXT("[LyraAbilityBridge:%s] %s"), *GetPathNameSafe(GetOwner()), *Message);
}

APawn* UActionCombatLyraAbilityBridgeComponent::ResolvePawnOwner() const
{
    return Cast<APawn>(GetOwner());
}

ULyraPawnExtensionComponent* UActionCombatLyraAbilityBridgeComponent::ResolvePawnExtensionComponent() const
{
    return ULyraPawnExtensionComponent::FindPawnExtensionComponent(ResolvePawnOwner());
}

UActionCombatComponent* UActionCombatLyraAbilityBridgeComponent::ResolveActionCombatComponent() const
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

ULyraAbilitySystemComponent* UActionCombatLyraAbilityBridgeComponent::ResolveLyraAbilitySystemComponent() const
{
    const APawn* Pawn = ResolvePawnOwner();
    const ULyraPawnExtensionComponent* PawnExtension = ULyraPawnExtensionComponent::FindPawnExtensionComponent(Pawn);
    return PawnExtension ? PawnExtension->GetLyraAbilitySystemComponent() : nullptr;
}

const FActionCombatLyraActionEventBinding* UActionCombatLyraAbilityBridgeComponent::FindActionEventBinding(const FGameplayTag& ActionTag) const
{
    for (const FActionCombatLyraActionEventBinding& Binding : ActionEventBindings)
    {
        if (Binding.ActionTag == ActionTag)
        {
            return &Binding;
        }
    }

    return nullptr;
}

FGameplayTag UActionCombatLyraAbilityBridgeComponent::ResolveStartedEventTag(const FGameplayTag& ActionTag) const
{
    if (const FActionCombatLyraActionEventBinding* Binding = FindActionEventBinding(ActionTag))
    {
        if (Binding->StartedEventTag.IsValid())
        {
            return Binding->StartedEventTag;
        }
    }

    if (DefaultActionStartedEventTag.IsValid())
    {
        return DefaultActionStartedEventTag;
    }

    return FGameplayTag::RequestGameplayTag(TEXT("Combat.GameplayEvent.Action.Started"), false);
}

FGameplayTag UActionCombatLyraAbilityBridgeComponent::ResolveEndedEventTag(const FGameplayTag& ActionTag) const
{
    if (const FActionCombatLyraActionEventBinding* Binding = FindActionEventBinding(ActionTag))
    {
        if (Binding->EndedEventTag.IsValid())
        {
            return Binding->EndedEventTag;
        }
    }

    if (DefaultActionEndedEventTag.IsValid())
    {
        return DefaultActionEndedEventTag;
    }

    return FGameplayTag::RequestGameplayTag(TEXT("Combat.GameplayEvent.Action.Ended"), false);
}

void UActionCombatLyraAbilityBridgeComponent::HandleObservedDashAbilityTagChanged(const FGameplayTag ChangedTag, int32 NewCount)
{
    bObservedDashAbilityTagActive = (ChangedTag == ObservedDashAbilityTag) && (NewCount > 0);
    SetMirroredDashStateActive(bObservedDashAbilityTagActive || (ActiveObservedDashAbilityCount > 0));
}

void UActionCombatLyraAbilityBridgeComponent::HandleObservedAbilityActivated(UGameplayAbility* Ability)
{
    if (!IsObservedDashAbility(Ability))
    {
        return;
    }

    ++ActiveObservedDashAbilityCount;
    LogBridge(FString::Printf(TEXT("Observed dash ability activated. Ability=%s ActiveCount=%d"), *GetNameSafe(Ability), ActiveObservedDashAbilityCount));
    SetMirroredDashStateActive(true);
}

void UActionCombatLyraAbilityBridgeComponent::HandleObservedAbilityEnded(UGameplayAbility* Ability)
{
    if (!IsObservedDashAbility(Ability))
    {
        return;
    }

    ActiveObservedDashAbilityCount = FMath::Max(ActiveObservedDashAbilityCount - 1, 0);
    LogBridge(FString::Printf(TEXT("Observed dash ability ended. Ability=%s ActiveCount=%d"), *GetNameSafe(Ability), ActiveObservedDashAbilityCount));
    SetMirroredDashStateActive(bObservedDashAbilityTagActive || (ActiveObservedDashAbilityCount > 0));
}

void UActionCombatLyraAbilityBridgeComponent::HandleAbilitySystemInitialized()
{
    BindLyraAbilitySystemComponent(ResolveLyraAbilitySystemComponent());
}

void UActionCombatLyraAbilityBridgeComponent::HandleAbilitySystemUninitialized()
{
    UnbindLyraAbilitySystemComponent();
}
