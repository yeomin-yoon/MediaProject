#include "ActionCombatLyraInputBridgeComponent.h"

#include "ActionCombatComponent.h"
#include "ActionCombatLyraGuardComponent.h"
#include "ActionCombatRuntimeLog.h"

#include "Character/LyraHeroComponent.h"
#include "Character/LyraPawnData.h"
#include "Character/LyraPawnExtensionComponent.h"
#include "Components/InputComponent.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Input/LyraInputConfig.h"
#include "InputAction.h"
#include "InputCoreTypes.h"

namespace ActionCombatLyraInputBridge
{
    static FGameplayTag GetPrimaryAttackInputTag()
    {
        static const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(TEXT("InputTag.Combat.Attack.Primary"), false);
        return Tag;
    }

    static FGameplayTag GetSecondaryAttackInputTag()
    {
        static const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(TEXT("InputTag.Combat.Attack.Secondary"), false);
        return Tag;
    }

    static FGameplayTag GetModifierInputTag()
    {
        static const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(TEXT("InputTag.Combat.Modifier"), false);
        return Tag;
    }

    static FGameplayTag GetLightCommandTag()
    {
        static const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(TEXT("Combat.Command.Light"), false);
        return Tag;
    }

    static FGameplayTag GetAltCommandTag()
    {
        static const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(TEXT("Combat.Command.Alt"), false);
        return Tag;
    }

    static FGameplayTag ResolveStartedCommandTag(const FActionCombatLyraInputBinding& Binding)
    {
        if (Binding.StartedCommandTag.IsValid())
        {
            return Binding.StartedCommandTag;
        }

        if (Binding.InputTag == GetPrimaryAttackInputTag())
        {
            return GetLightCommandTag();
        }

        if (Binding.InputTag == GetSecondaryAttackInputTag())
        {
            return GetAltCommandTag();
        }

        return Binding.StartedCommandTag;
    }

    static const UInputAction* ResolveInputActionOverride(const FGameplayTag& InputTag)
    {
        if (InputTag == GetSecondaryAttackInputTag())
        {
            return LoadObject<UInputAction>(nullptr, TEXT("/Game/1dev/OS/IA_TestHero_Combat_Secondary.IA_TestHero_Combat_Secondary"));
        }

        if (InputTag == GetModifierInputTag())
        {
            return LoadObject<UInputAction>(nullptr, TEXT("/Game/1dev/OS/IA_TestHero_Combat_Modifier.IA_TestHero_Combat_Modifier"));
        }

        return nullptr;
    }
}

UActionCombatLyraInputBridgeComponent::UActionCombatLyraInputBridgeComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UActionCombatLyraInputBridgeComponent::BeginPlay()
{
    Super::BeginPlay();
    TryBindInput();
}

void UActionCombatLyraInputBridgeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    RemoveInputBindings();
    Super::EndPlay(EndPlayReason);
}

void UActionCombatLyraInputBridgeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    APawn* Pawn = ResolvePawnOwner();
    if (!Pawn)
    {
        RemoveInputBindings();
        return;
    }

    if (bBindOnlyLocallyControlled && !Pawn->IsLocallyControlled())
    {
        RemoveInputBindings();
        return;
    }

    if (BoundInputComponent.Get() != Pawn->InputComponent)
    {
        RemoveInputBindings();
    }

    if (!BoundInputComponent.IsValid())
    {
        TryBindInput();
    }

    PollRawSecondaryInput();
}

void UActionCombatLyraInputBridgeComponent::RefreshInputBindings()
{
    RemoveInputBindings();
    TryBindInput();
}

void UActionCombatLyraInputBridgeComponent::TryBindInput()
{
    APawn* Pawn = ResolvePawnOwner();
    if (!Pawn)
    {
        return;
    }

    if (bBindOnlyLocallyControlled && !Pawn->IsLocallyControlled())
    {
        return;
    }

    ULyraHeroComponent* HeroComponent = Pawn->FindComponentByClass<ULyraHeroComponent>();
    if (!HeroComponent || !HeroComponent->IsReadyToBindInputs())
    {
        return;
    }

    UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(Pawn->InputComponent);
    if (!EnhancedInputComponent)
    {
        return;
    }

    const ULyraInputConfig* InputConfig = ResolveInputConfig();
    if (!InputConfig)
    {
        return;
    }

    if (InputBindings.IsEmpty())
    {
        LogBinding(TEXT("No input bindings configured."));
        BoundInputComponent = EnhancedInputComponent;
        return;
    }

    for (const FActionCombatLyraInputBinding& Binding : InputBindings)
    {
        if (!Binding.InputTag.IsValid())
        {
            continue;
        }

        const UInputAction* InputAction = ResolveInputActionForTag(InputConfig, Binding.InputTag);
        if (!InputAction)
        {
            LogBinding(FString::Printf(TEXT("Input action not found for InputTag=%s"), *Binding.InputTag.ToString()));
            continue;
        }

        const bool bWantsStartedBinding = Binding.WantsStartedBinding()
            || ActionCombatLyraInputBridge::ResolveStartedCommandTag(Binding).IsValid();

        if (bWantsStartedBinding)
        {
            BoundInputHandles.Add(EnhancedInputComponent->BindAction(InputAction, ETriggerEvent::Started, this, &ThisClass::HandleInputStarted, Binding.InputTag).GetHandle());
        }

        if (Binding.WantsTriggeredBinding())
        {
            BoundInputHandles.Add(EnhancedInputComponent->BindAction(InputAction, ETriggerEvent::Triggered, this, &ThisClass::HandleInputTriggered, Binding.InputTag).GetHandle());
        }

        if (Binding.WantsCompletedBinding())
        {
            BoundInputHandles.Add(EnhancedInputComponent->BindAction(InputAction, ETriggerEvent::Completed, this, &ThisClass::HandleInputCompleted, Binding.InputTag).GetHandle());
        }
    }

    BoundInputComponent = EnhancedInputComponent;
    if (bPollRightMouseButtonForSecondary && FindBindingByInputTag(ActionCombatLyraInputBridge::GetSecondaryAttackInputTag()))
    {
        BindRawSecondaryKeyInput(EnhancedInputComponent);

        if (APlayerController* PlayerController = Cast<APlayerController>(Pawn->GetController()))
        {
            BindRawSecondaryKeyInput(PlayerController->InputComponent);
        }
    }

    LogBinding(FString::Printf(TEXT("Bound %d combat input handles."), BoundInputHandles.Num()));
}

void UActionCombatLyraInputBridgeComponent::RemoveInputBindings()
{
    UnbindRawSecondaryKeyInput();

    if (UEnhancedInputComponent* InputComponent = Cast<UEnhancedInputComponent>(BoundInputComponent.Get()))
    {
        for (const uint32 Handle : BoundInputHandles)
        {
            InputComponent->RemoveBindingByHandle(Handle);
        }
    }

    BoundInputHandles.Reset();
    BoundInputComponent.Reset();
    NextRepeatCommandTimeByInputTag.Reset();
    LastStartedInputTimeByInputTag.Reset();
    bWasRawSecondaryInputDown = false;
}

void UActionCombatLyraInputBridgeComponent::HandleInputStarted(FGameplayTag InputTag)
{
    if (const FActionCombatLyraInputBinding* Binding = FindBindingByInputTag(InputTag))
    {
        const double CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
        if (const double* LastStartedTime = LastStartedInputTimeByInputTag.Find(InputTag))
        {
            if (CurrentTime - *LastStartedTime <= 0.02)
            {
                LogBinding(FString::Printf(TEXT("Started input ignored as duplicate. InputTag=%s"), *InputTag.ToString()));
                return;
            }
        }

        LastStartedInputTimeByInputTag.Add(InputTag, CurrentTime);

        ApplyStartedBinding(*Binding);

        if (Binding->bRepeatStartedCommandWhileHeld)
        {
            NextRepeatCommandTimeByInputTag.Add(InputTag, CurrentTime + FMath::Max(0.03f, Binding->StartedCommandRepeatIntervalSeconds));
        }
    }
}

void UActionCombatLyraInputBridgeComponent::HandleInputTriggered(FGameplayTag InputTag)
{
    const FActionCombatLyraInputBinding* Binding = FindBindingByInputTag(InputTag);
    if (!Binding || !Binding->bRepeatStartedCommandWhileHeld)
    {
        return;
    }

    const double CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
    const double* NextRepeatTime = NextRepeatCommandTimeByInputTag.Find(InputTag);
    if (!NextRepeatTime)
    {
        NextRepeatCommandTimeByInputTag.Add(InputTag, CurrentTime + FMath::Max(0.03f, Binding->StartedCommandRepeatIntervalSeconds));
        return;
    }

    if (CurrentTime < *NextRepeatTime)
    {
        return;
    }

    ApplyRepeatedStartedCommand(*Binding);
    NextRepeatCommandTimeByInputTag.Add(InputTag, CurrentTime + FMath::Max(0.03f, Binding->StartedCommandRepeatIntervalSeconds));
}

void UActionCombatLyraInputBridgeComponent::HandleInputCompleted(FGameplayTag InputTag)
{
    if (const FActionCombatLyraInputBinding* Binding = FindBindingByInputTag(InputTag))
    {
        ApplyCompletedBinding(*Binding);
        NextRepeatCommandTimeByInputTag.Remove(InputTag);
    }
}

void UActionCombatLyraInputBridgeComponent::BindRawSecondaryKeyInput(UInputComponent* InputComponent)
{
    if (!InputComponent)
    {
        return;
    }

    for (const FRawSecondaryKeyBinding& ExistingBinding : RawSecondaryKeyBindings)
    {
        if (ExistingBinding.InputComponent.Get() == InputComponent)
        {
            return;
        }
    }

    FInputKeyBinding& PressedBinding = InputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &ThisClass::HandleRawSecondaryPressed);
    PressedBinding.bConsumeInput = false;

    FInputKeyBinding& ReleasedBinding = InputComponent->BindKey(EKeys::RightMouseButton, IE_Released, this, &ThisClass::HandleRawSecondaryReleased);
    ReleasedBinding.bConsumeInput = false;

    FRawSecondaryKeyBinding RawBinding;
    RawBinding.InputComponent = InputComponent;
    RawBinding.PressedBindingIndex = InputComponent->KeyBindings.Num() - 2;
    RawBinding.ReleasedBindingIndex = InputComponent->KeyBindings.Num() - 1;
    RawSecondaryKeyBindings.Add(RawBinding);

    LogBinding(FString::Printf(TEXT("Bound raw RightMouseButton fallback on InputComponent=%s"), *GetPathNameSafe(InputComponent)));
}

void UActionCombatLyraInputBridgeComponent::UnbindRawSecondaryKeyInput()
{
    for (const FRawSecondaryKeyBinding& RawBinding : RawSecondaryKeyBindings)
    {
        UInputComponent* InputComponent = RawBinding.InputComponent.Get();
        if (!InputComponent)
        {
            continue;
        }

        TArray<int32> BindingIndices;
        BindingIndices.Add(RawBinding.PressedBindingIndex);
        BindingIndices.Add(RawBinding.ReleasedBindingIndex);
        BindingIndices.Sort(TGreater<int32>());

        for (const int32 BindingIndex : BindingIndices)
        {
            if (!InputComponent->KeyBindings.IsValidIndex(BindingIndex))
            {
                continue;
            }

            const FInputKeyBinding& KeyBinding = InputComponent->KeyBindings[BindingIndex];
            if (KeyBinding.Chord.Key == EKeys::RightMouseButton
                && (KeyBinding.KeyEvent == IE_Pressed || KeyBinding.KeyEvent == IE_Released))
            {
                InputComponent->KeyBindings.RemoveAt(BindingIndex);
            }
        }
    }

    RawSecondaryKeyBindings.Reset();
}

void UActionCombatLyraInputBridgeComponent::HandleRawSecondaryPressed()
{
    const FGameplayTag SecondaryInputTag = ActionCombatLyraInputBridge::GetSecondaryAttackInputTag();
    LogBinding(TEXT("Raw RightMouseButton key pressed -> InputTag.Combat.Attack.Secondary"));
    bWasRawSecondaryInputDown = true;
    HandleInputStarted(SecondaryInputTag);
}

void UActionCombatLyraInputBridgeComponent::HandleRawSecondaryReleased()
{
    const FGameplayTag SecondaryInputTag = ActionCombatLyraInputBridge::GetSecondaryAttackInputTag();
    LogBinding(TEXT("Raw RightMouseButton key released -> InputTag.Combat.Attack.Secondary"));
    bWasRawSecondaryInputDown = false;
    HandleInputCompleted(SecondaryInputTag);
}

void UActionCombatLyraInputBridgeComponent::PollRawSecondaryInput()
{
    if (!bPollRightMouseButtonForSecondary)
    {
        return;
    }

    if (!BoundInputComponent.IsValid())
    {
        return;
    }

    const FGameplayTag SecondaryInputTag = ActionCombatLyraInputBridge::GetSecondaryAttackInputTag();
    if (!FindBindingByInputTag(SecondaryInputTag))
    {
        return;
    }

    APawn* Pawn = ResolvePawnOwner();
    APlayerController* PlayerController = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
    if (!Pawn || !Pawn->IsLocallyControlled() || !PlayerController)
    {
        bWasRawSecondaryInputDown = false;
        return;
    }

    const bool bIsDown = PlayerController->IsInputKeyDown(EKeys::RightMouseButton);
    if (bIsDown && !bWasRawSecondaryInputDown)
    {
        LogBinding(TEXT("Raw RightMouseButton pressed -> InputTag.Combat.Attack.Secondary"));
        HandleInputStarted(SecondaryInputTag);
    }
    else if (!bIsDown && bWasRawSecondaryInputDown)
    {
        LogBinding(TEXT("Raw RightMouseButton released -> InputTag.Combat.Attack.Secondary"));
        HandleInputCompleted(SecondaryInputTag);
    }

    bWasRawSecondaryInputDown = bIsDown;

    if (bIsDown)
    {
        ProcessRawHeldRepeat(SecondaryInputTag);
    }
}

void UActionCombatLyraInputBridgeComponent::ProcessRawHeldRepeat(FGameplayTag InputTag)
{
    const FActionCombatLyraInputBinding* Binding = FindBindingByInputTag(InputTag);
    if (!Binding || !Binding->bRepeatStartedCommandWhileHeld)
    {
        return;
    }

    const double CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
    const double* NextRepeatTime = NextRepeatCommandTimeByInputTag.Find(InputTag);
    if (!NextRepeatTime)
    {
        NextRepeatCommandTimeByInputTag.Add(InputTag, CurrentTime + FMath::Max(0.03f, Binding->StartedCommandRepeatIntervalSeconds));
        return;
    }

    if (CurrentTime < *NextRepeatTime)
    {
        return;
    }

    ApplyRepeatedStartedCommand(*Binding);
    NextRepeatCommandTimeByInputTag.Add(InputTag, CurrentTime + FMath::Max(0.03f, Binding->StartedCommandRepeatIntervalSeconds));
}

void UActionCombatLyraInputBridgeComponent::ApplyStartedBinding(const FActionCombatLyraInputBinding& Binding)
{
    if (Binding.bSetGuardInputHeldOnStarted)
    {
        if (UActionCombatLyraGuardComponent* GuardComponent = ResolveGuardComponent())
        {
            GuardComponent->SetGuardInputHeld(true);
        }
        else
        {
            LogBinding(FString::Printf(TEXT("Started input could not enable guard because GuardComponent was missing. InputTag=%s"), *Binding.InputTag.ToString()));
        }
    }

    UActionCombatComponent* CombatComponent = ResolveActionCombatComponent();
    if (!CombatComponent)
    {
        if (Binding.StartedCommandTag.IsValid() || (Binding.bMirrorHeldStateWhilePressed && Binding.HeldInputStateTag.IsValid()) || Binding.bSetFocusActiveOnStarted)
        {
            LogBinding(FString::Printf(TEXT("Started input ignored because ActionCombatComponent was missing. InputTag=%s"), *Binding.InputTag.ToString()));
        }
        return;
    }

    if (Binding.bMirrorHeldStateWhilePressed && Binding.HeldInputStateTag.IsValid())
    {
        CombatComponent->SetInputStateTagActive(Binding.HeldInputStateTag, true);
    }

    if (Binding.bSetFocusActiveOnStarted)
    {
        CombatComponent->SetFocusActive(true);
    }

    const FGameplayTag StartedCommandTag = ActionCombatLyraInputBridge::ResolveStartedCommandTag(Binding);
    if (StartedCommandTag.IsValid())
    {
        if (StartedCommandTag != Binding.StartedCommandTag)
        {
            LogBinding(FString::Printf(TEXT("Started command remapped InputTag=%s From=%s To=%s"), *Binding.InputTag.ToString(), *Binding.StartedCommandTag.ToString(), *StartedCommandTag.ToString()));
        }

        CombatComponent->RequestCommand(StartedCommandTag);
    }
}

void UActionCombatLyraInputBridgeComponent::ApplyRepeatedStartedCommand(const FActionCombatLyraInputBinding& Binding)
{
    const FGameplayTag StartedCommandTag = ActionCombatLyraInputBridge::ResolveStartedCommandTag(Binding);
    if (!StartedCommandTag.IsValid())
    {
        return;
    }

    if (UActionCombatComponent* CombatComponent = ResolveActionCombatComponent())
    {
        CombatComponent->RequestCommand(StartedCommandTag);
    }
    else
    {
        LogBinding(FString::Printf(TEXT("Repeated input ignored because ActionCombatComponent was missing. InputTag=%s"), *Binding.InputTag.ToString()));
    }
}

void UActionCombatLyraInputBridgeComponent::ApplyCompletedBinding(const FActionCombatLyraInputBinding& Binding)
{
    if (Binding.bClearGuardInputHeldOnCompleted)
    {
        if (UActionCombatLyraGuardComponent* GuardComponent = ResolveGuardComponent())
        {
            GuardComponent->SetGuardInputHeld(false);
        }
        else
        {
            LogBinding(FString::Printf(TEXT("Completed input could not disable guard because GuardComponent was missing. InputTag=%s"), *Binding.InputTag.ToString()));
        }
    }

    UActionCombatComponent* CombatComponent = ResolveActionCombatComponent();
    if (!CombatComponent)
    {
        if (Binding.CompletedCommandTag.IsValid() || (Binding.bMirrorHeldStateWhilePressed && Binding.HeldInputStateTag.IsValid()) || Binding.bSetFocusInactiveOnCompleted)
        {
            LogBinding(FString::Printf(TEXT("Completed input ignored because ActionCombatComponent was missing. InputTag=%s"), *Binding.InputTag.ToString()));
        }
        return;
    }

    if (Binding.bMirrorHeldStateWhilePressed && Binding.HeldInputStateTag.IsValid())
    {
        CombatComponent->SetInputStateTagActive(Binding.HeldInputStateTag, false);
    }

    if (Binding.bSetFocusInactiveOnCompleted)
    {
        CombatComponent->SetFocusActive(false);
    }

    if (Binding.CompletedCommandTag.IsValid())
    {
        CombatComponent->RequestCommand(Binding.CompletedCommandTag);
    }
}

void UActionCombatLyraInputBridgeComponent::LogBinding(const FString& Message) const
{
    if (!bLogBindingFlow)
    {
        return;
    }

    UE_LOG(LogActionCombatRuntime, Log, TEXT("[LyraBridge:%s] %s"), *GetPathNameSafe(GetOwner()), *Message);
}

APawn* UActionCombatLyraInputBridgeComponent::ResolvePawnOwner() const
{
    return Cast<APawn>(GetOwner());
}

UActionCombatComponent* UActionCombatLyraInputBridgeComponent::ResolveActionCombatComponent() const
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

UActionCombatLyraGuardComponent* UActionCombatLyraInputBridgeComponent::ResolveGuardComponent() const
{
    if (AActor* Owner = GetOwner())
    {
        if (UActionCombatLyraGuardComponent* ExplicitComponent = Cast<UActionCombatLyraGuardComponent>(GuardComponentReference.GetComponent(Owner)))
        {
            return ExplicitComponent;
        }

        return Owner->FindComponentByClass<UActionCombatLyraGuardComponent>();
    }

    return nullptr;
}

const ULyraInputConfig* UActionCombatLyraInputBridgeComponent::ResolveInputConfig() const
{
    const APawn* Pawn = ResolvePawnOwner();
    if (!Pawn)
    {
        return nullptr;
    }

    const ULyraPawnExtensionComponent* PawnExtension = ULyraPawnExtensionComponent::FindPawnExtensionComponent(Pawn);
    const ULyraPawnData* PawnData = PawnExtension ? PawnExtension->GetPawnData<ULyraPawnData>() : nullptr;
    return PawnData ? PawnData->InputConfig : nullptr;
}

const UInputAction* UActionCombatLyraInputBridgeComponent::ResolveInputActionForTag(const ULyraInputConfig* InputConfig, const FGameplayTag& InputTag) const
{
    if (!InputTag.IsValid())
    {
        return nullptr;
    }

    if (const UInputAction* InputActionOverride = ActionCombatLyraInputBridge::ResolveInputActionOverride(InputTag))
    {
        LogBinding(FString::Printf(TEXT("Using action-combat input override for InputTag=%s"), *InputTag.ToString()));
        return InputActionOverride;
    }

    if (!InputConfig)
    {
        return nullptr;
    }

    for (const FLyraInputAction& NativeAction : InputConfig->NativeInputActions)
    {
        if (NativeAction.InputAction && (NativeAction.InputTag == InputTag))
        {
            return NativeAction.InputAction;
        }
    }

    if (!bSearchAbilityInputActionsAsFallback)
    {
        return nullptr;
    }

    for (const FLyraInputAction& AbilityAction : InputConfig->AbilityInputActions)
    {
        if (AbilityAction.InputAction && (AbilityAction.InputTag == InputTag))
        {
            LogBinding(FString::Printf(TEXT("Using ability-input fallback for InputTag=%s"), *InputTag.ToString()));
            return AbilityAction.InputAction;
        }
    }

    return nullptr;
}

const FActionCombatLyraInputBinding* UActionCombatLyraInputBridgeComponent::FindBindingByInputTag(const FGameplayTag& InputTag) const
{
    for (const FActionCombatLyraInputBinding& Binding : InputBindings)
    {
        if (Binding.InputTag == InputTag)
        {
            return &Binding;
        }
    }

    return nullptr;
}
