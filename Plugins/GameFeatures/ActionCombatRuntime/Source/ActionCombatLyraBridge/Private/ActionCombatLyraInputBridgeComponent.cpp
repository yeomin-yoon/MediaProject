#include "ActionCombatLyraInputBridgeComponent.h"

#include "ActionCombatComponent.h"
#include "ActionCombatRuntimeLog.h"

#include "Character/LyraHeroComponent.h"
#include "Character/LyraPawnData.h"
#include "Character/LyraPawnExtensionComponent.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/Pawn.h"
#include "Input/LyraInputConfig.h"
#include "InputAction.h"

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

        if (Binding.WantsStartedBinding())
        {
            BoundInputHandles.Add(EnhancedInputComponent->BindAction(InputAction, ETriggerEvent::Started, this, &ThisClass::HandleInputStarted, Binding.InputTag).GetHandle());
        }

        if (Binding.WantsCompletedBinding())
        {
            BoundInputHandles.Add(EnhancedInputComponent->BindAction(InputAction, ETriggerEvent::Completed, this, &ThisClass::HandleInputCompleted, Binding.InputTag).GetHandle());
        }
    }

    BoundInputComponent = EnhancedInputComponent;
    LogBinding(FString::Printf(TEXT("Bound %d combat input handles."), BoundInputHandles.Num()));
}

void UActionCombatLyraInputBridgeComponent::RemoveInputBindings()
{
    if (UEnhancedInputComponent* InputComponent = Cast<UEnhancedInputComponent>(BoundInputComponent.Get()))
    {
        for (const uint32 Handle : BoundInputHandles)
        {
            InputComponent->RemoveBindingByHandle(Handle);
        }
    }

    BoundInputHandles.Reset();
    BoundInputComponent.Reset();
}

void UActionCombatLyraInputBridgeComponent::HandleInputStarted(FGameplayTag InputTag)
{
    if (const FActionCombatLyraInputBinding* Binding = FindBindingByInputTag(InputTag))
    {
        ApplyStartedBinding(*Binding);
    }
}

void UActionCombatLyraInputBridgeComponent::HandleInputCompleted(FGameplayTag InputTag)
{
    if (const FActionCombatLyraInputBinding* Binding = FindBindingByInputTag(InputTag))
    {
        ApplyCompletedBinding(*Binding);
    }
}

void UActionCombatLyraInputBridgeComponent::ApplyStartedBinding(const FActionCombatLyraInputBinding& Binding)
{
    UActionCombatComponent* CombatComponent = ResolveActionCombatComponent();
    if (!CombatComponent)
    {
        LogBinding(FString::Printf(TEXT("Started input ignored because ActionCombatComponent was missing. InputTag=%s"), *Binding.InputTag.ToString()));
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

    if (Binding.StartedCommandTag.IsValid())
    {
        CombatComponent->RequestCommand(Binding.StartedCommandTag);
    }
}

void UActionCombatLyraInputBridgeComponent::ApplyCompletedBinding(const FActionCombatLyraInputBinding& Binding)
{
    UActionCombatComponent* CombatComponent = ResolveActionCombatComponent();
    if (!CombatComponent)
    {
        LogBinding(FString::Printf(TEXT("Completed input ignored because ActionCombatComponent was missing. InputTag=%s"), *Binding.InputTag.ToString()));
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
    if (!InputConfig || !InputTag.IsValid())
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
