#include "ActionCombatLyraAbilityBridgeComponent.h"

#include "ActionCombatBlueprintLibrary.h"
#include "ActionCombatComponent.h"
#include "ActionCombatMeleeTraceComponent.h"
#include "ActionCombatRuntimeLog.h"

#include "AbilitySystem/LyraAbilitySystemComponent.h"
#include "Character/LyraPawnExtensionComponent.h"
#include "GameFramework/Pawn.h"

UActionCombatLyraAbilityBridgeComponent::UActionCombatLyraAbilityBridgeComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    PrimaryComponentTick.bCanEverTick = false;
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UActionCombatLyraAbilityBridgeComponent::BeginPlay()
{
    Super::BeginPlay();
    TryBindCombatComponent();
}

void UActionCombatLyraAbilityBridgeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    UnbindCombatComponent();
    Super::EndPlay(EndPlayReason);
}

void UActionCombatLyraAbilityBridgeComponent::RefreshCombatBinding()
{
    UnbindCombatComponent();
    TryBindCombatComponent();
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

void UActionCombatLyraAbilityBridgeComponent::HandleActionStarted(FActionCombatActiveActionState ActionState)
{
    if (!bDispatchActionStartedEvents)
    {
        return;
    }

    const FGameplayTag EventTag = ResolveStartedEventTag(ActionState.ActionTag);
    DispatchEventForActionState(EventTag, ActionState, false);
}

void UActionCombatLyraAbilityBridgeComponent::HandleActionEnded(FActionCombatActiveActionState ActionState)
{
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
        return Binding->StartedEventTag;
    }

    return DefaultActionStartedEventTag;
}

FGameplayTag UActionCombatLyraAbilityBridgeComponent::ResolveEndedEventTag(const FGameplayTag& ActionTag) const
{
    if (const FActionCombatLyraActionEventBinding* Binding = FindActionEventBinding(ActionTag))
    {
        return Binding->EndedEventTag;
    }

    return DefaultActionEndedEventTag;
}
