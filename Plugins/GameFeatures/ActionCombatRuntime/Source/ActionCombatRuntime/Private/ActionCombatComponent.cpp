#include "ActionCombatComponent.h"

#include "ActionCombatRuntimeLog.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/GameStateBase.h"
#include "Net/UnrealNetwork.h"

namespace ActionCombatComponent
{
    static const FName BaseStyleLayerId(TEXT("BaseStyle"));
}

UActionCombatComponent::UActionCombatComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    SetIsReplicatedByDefault(true);
}

void UActionCombatComponent::BeginPlay()
{
    Super::BeginPlay();

    SetComponentTickEnabled(false);
    SortStyleLayers();
    UpdateReplicatedStateFromLocal();
}

void UActionCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!HasRuntimeAuthority())
    {
        return;
    }

    if (!ActiveActionState.ActionTag.IsValid() || !ActiveActionDefinition)
    {
        SetComponentTickEnabled(false);
        return;
    }

    UpdateActiveActionProgress(DeltaTime);
    TryCommitPendingCommands();
}

void UActionCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ThisClass, ReplicatedState);
}

void UActionCombatComponent::SetBaseStyle(UActionCombatStyleData* StyleData)
{
    if (!HasRuntimeAuthority())
    {
        return;
    }

    SetStyleLayer(ActionCombatComponent::BaseStyleLayerId, StyleData, 0, 1.0f);
}

void UActionCombatComponent::SetStyleLayer(FName LayerId, UActionCombatStyleData* StyleData, int32 Priority, float LayerPlayRateMultiplier)
{
    if (!HasRuntimeAuthority())
    {
        return;
    }

    if (LayerId.IsNone())
    {
        UE_LOG(LogActionCombatRuntime, Warning, TEXT("SetStyleLayer ignored because LayerId was None on %s."), *GetPathName());
        return;
    }

    if (!StyleData)
    {
        ClearStyleLayer(LayerId);
        return;
    }

    for (FActionCombatStyleLayerEntry& Entry : StyleLayers)
    {
        if (Entry.LayerId == LayerId)
        {
            Entry.Style = StyleData;
            Entry.Priority = Priority;
            Entry.LayerPlayRateMultiplier = FMath::Max(LayerPlayRateMultiplier, 0.01f);
            Entry.bEnabled = true;
            SortStyleLayers();
            return;
        }
    }

    FActionCombatStyleLayerEntry& NewEntry = StyleLayers.AddDefaulted_GetRef();
    NewEntry.LayerId = LayerId;
    NewEntry.Priority = Priority;
    NewEntry.Style = StyleData;
    NewEntry.LayerPlayRateMultiplier = FMath::Max(LayerPlayRateMultiplier, 0.01f);
    NewEntry.bEnabled = true;

    SortStyleLayers();
}

void UActionCombatComponent::ClearStyleLayer(FName LayerId)
{
    if (!HasRuntimeAuthority())
    {
        return;
    }

    StyleLayers.RemoveAll([LayerId](const FActionCombatStyleLayerEntry& Entry)
    {
        return Entry.LayerId == LayerId;
    });
}

void UActionCombatComponent::ClearAllStyleLayers()
{
    if (!HasRuntimeAuthority())
    {
        return;
    }

    StyleLayers.Reset();
}

void UActionCombatComponent::SetRuntimePlayRateMultiplier(float NewMultiplier)
{
    if (!HasRuntimeAuthority())
    {
        return;
    }

    RuntimePlayRateMultiplier = FMath::Max(NewMultiplier, 0.01f);

    if (ActiveActionDefinition)
    {
        ActiveActionState.EffectivePlayRate = ActiveActionDefinition->BasePlayRate * ActiveActionStylePlayRateSnapshot * RuntimePlayRateMultiplier;
        RefreshActiveMontagePlayRate();
        UpdateReplicatedStateFromLocal();
    }
}

bool UActionCombatComponent::RequestCommand(FGameplayTag CommandTag)
{
    if (!CommandTag.IsValid())
    {
        return false;
    }

    return RequestCommandWithHeldTags(CommandTag, HeldInputTags);
}

bool UActionCombatComponent::RequestCommandWithHeldTags(FGameplayTag CommandTag, FGameplayTagContainer HeldInputTagsSnapshot)
{
    if (!CommandTag.IsValid())
    {
        return false;
    }

    FActionCombatBufferedCommandState CommandRequest;
    CommandRequest.CommandTag = CommandTag;
    CommandRequest.HeldInputTags = HeldInputTagsSnapshot;

    AppendCommandHistory(CommandRequest);
    LogCommandFlow(FString::Printf(TEXT("RequestCommand %s History=%s"), *FormatCommandState(CommandRequest), *BuildCommandHistoryString()));

    if (!HasRuntimeAuthority())
    {
        ServerRequestCommand(CommandTag, HeldInputTagsSnapshot);
        return true;
    }

    return HandleCommandRequestInternal(CommandRequest);
}

void UActionCombatComponent::SetInputStateTagActive(FGameplayTag InputStateTag, bool bNewActiveState)
{
    if (!InputStateTag.IsValid())
    {
        return;
    }

    if (bNewActiveState)
    {
        HeldInputTags.AddTag(InputStateTag);
    }
    else
    {
        HeldInputTags.RemoveTag(InputStateTag);
    }

    LogCommandFlow(FString::Printf(TEXT("InputState %s %s"), bNewActiveState ? TEXT("Pressed") : TEXT("Released"), *InputStateTag.ToString()));
}

bool UActionCombatComponent::SetFocusActive(bool bNewFocusActive)
{
    if (!HasRuntimeAuthority())
    {
        ServerSetFocusActive(bNewFocusActive);
        return true;
    }

    if (bFocusActive == bNewFocusActive)
    {
        return false;
    }

    bFocusActive = bNewFocusActive;
    UpdateReplicatedStateFromLocal();
    OnFocusChanged.Broadcast(bFocusActive);
    return true;
}

void UActionCombatComponent::ClearPendingCommands()
{
    if (!HasRuntimeAuthority())
    {
        return;
    }

    BufferedCommand.Reset();
    PendingInterruptCommand.Reset();
    UpdateReplicatedStateFromLocal();
    OnBufferedCommandChanged.Broadcast(BufferedCommand.CommandTag);
    LogCommandFlow(TEXT("ClearedPendingCommands"));
}

bool UActionCombatComponent::TryForceCommitBufferedCommand()
{
    if (!HasRuntimeAuthority())
    {
        return false;
    }

    if (PendingInterruptCommand.IsValid())
    {
        const FActionCombatBufferedCommandState InterruptCommand = PendingInterruptCommand;
        const FActionCombatBufferedCommandState SavedBufferedCommand = BufferedCommand;
        PendingInterruptCommand.Reset();
        BufferedCommand.Reset();
        const bool bStarted = ResolveTransitionAndStart(ActiveActionState.ActionTag, InterruptCommand);
        if (!bStarted)
        {
            BufferedCommand = SavedBufferedCommand;
        }
        else if (SavedBufferedCommand.IsValid())
        {
            OnBufferedCommandChanged.Broadcast(BufferedCommand.CommandTag);
        }

        UpdateReplicatedStateFromLocal();
        return bStarted;
    }

    if (BufferedCommand.IsValid())
    {
        const FActionCombatBufferedCommandState QueuedCommand = BufferedCommand;
        BufferedCommand.Reset();
        const bool bStarted = ResolveTransitionAndStart(ActiveActionState.ActionTag, QueuedCommand);
        UpdateReplicatedStateFromLocal();
        return bStarted;
    }

    return false;
}

void UActionCombatComponent::InterruptActiveAction()
{
    if (!HasRuntimeAuthority() || !ActiveActionState.ActionTag.IsValid())
    {
        return;
    }

    BufferedCommand.Reset();
    PendingInterruptCommand.Reset();
    OnBufferedCommandChanged.Broadcast(BufferedCommand.CommandTag);
    EndActiveAction(true);
    UpdateReplicatedStateFromLocal();
    LogCommandFlow(TEXT("ActiveActionInterrupted"));
}

bool UActionCombatComponent::StartActionFromTag(FGameplayTag ActionTag)
{
    if (!HasRuntimeAuthority())
    {
        return false;
    }

    const UActionCombatStyleData* SourceStyle = nullptr;
    const FActionCombatActionDefinition* ActionDefinition = FindActionDefinitionInLayers(ActionTag, SourceStyle);
    return StartActionFromDefinition(ActionDefinition, SourceStyle);
}

void UActionCombatComponent::ServerRequestCommand_Implementation(FGameplayTag CommandTag, FGameplayTagContainer HeldInputTagsSnapshot)
{
    FActionCombatBufferedCommandState CommandRequest;
    CommandRequest.CommandTag = CommandTag;
    CommandRequest.HeldInputTags = HeldInputTagsSnapshot;
    AppendCommandHistory(CommandRequest);
    HandleCommandRequestInternal(CommandRequest);
}

void UActionCombatComponent::ServerSetFocusActive_Implementation(bool bNewFocusActive)
{
    SetFocusActive(bNewFocusActive);
}

void UActionCombatComponent::OnRep_ReplicatedState()
{
    const FActionCombatActiveActionState PreviousState = ActiveActionState;

    bFocusActive = ReplicatedState.bFocusActive;
    BufferedCommand.CommandTag = ReplicatedState.BufferedCommandTag;
    BufferedCommand.HeldInputTags.Reset();
    PendingInterruptCommand.CommandTag = ReplicatedState.PendingInterruptCommandTag;
    PendingInterruptCommand.HeldInputTags.Reset();

    if (!ReplicatedState.ActiveActionTag.IsValid())
    {
        ActiveActionState.Reset();
    }
    else
    {
        ActiveActionState.ActionTag = ReplicatedState.ActiveActionTag;
        ActiveActionState.ActionInstanceId = ReplicatedState.ActionInstanceId;
        ActiveActionState.EffectivePlayRate = ReplicatedState.EffectivePlayRate;
        ActiveActionState.bStartedWhileFocusActive = bFocusActive;
        ActiveActionState.bUsingMontageTiming = ReplicatedState.ActiveMontage != nullptr;

        const UActionCombatStyleData* SourceStyle = nullptr;
        if (const FActionCombatActionDefinition* ReplicatedActionDefinition = FindActionDefinitionInLayers(ReplicatedState.ActiveActionTag, SourceStyle))
        {
            ActiveActionState.TraceSourceId = ReplicatedActionDefinition->TraceSourceId;
            ActiveActionState.HitWindowName = ReplicatedActionDefinition->HitWindowName;
            ActiveActionState.MotionValue = ReplicatedActionDefinition->MotionValue;
            ActiveActionState.PoiseDamage = ReplicatedActionDefinition->PoiseDamage;
            ActiveActionState.BuildupMultiplier = ReplicatedActionDefinition->BuildupMultiplier;
        }
        else
        {
            ActiveActionState.TraceSourceId = NAME_None;
            ActiveActionState.HitWindowName = NAME_None;
            ActiveActionState.MotionValue = 1.0f;
            ActiveActionState.PoiseDamage = 0.0f;
            ActiveActionState.BuildupMultiplier = 1.0f;
        }
    }

    if (!HasRuntimeAuthority())
    {
        SyncReplicatedMontage();
    }

    if (PreviousState.ActionInstanceId != ActiveActionState.ActionInstanceId)
    {
        if (PreviousState.ActionTag.IsValid())
        {
            OnActionEnded.Broadcast(PreviousState);
        }

        if (ActiveActionState.ActionTag.IsValid())
        {
            OnActionStarted.Broadcast(ActiveActionState);
        }
    }
}

bool UActionCombatComponent::HasRuntimeAuthority() const
{
    if (!bAuthorityOnly)
    {
        return true;
    }

    const AActor* Owner = GetOwner();
    return Owner != nullptr && Owner->HasAuthority();
}

bool UActionCombatComponent::HandleCommandRequestInternal(const FActionCombatBufferedCommandState& CommandRequest)
{
    if (!ActiveActionState.ActionTag.IsValid())
    {
        const bool bStarted = ResolveTransitionAndStart(FGameplayTag(), CommandRequest);
        UpdateReplicatedStateFromLocal();
        if (!bStarted)
        {
            LogCommandFlow(FString::Printf(TEXT("RequestRejected Neutral Command=%s"), *FormatCommandState(CommandRequest)));
        }
        return bStarted;
    }

    if (DodgeCommandTag.IsValid() && CommandRequest.CommandTag == DodgeCommandTag)
    {
        PendingInterruptCommand = CommandRequest;
        UpdateReplicatedStateFromLocal();
        LogCommandFlow(FString::Printf(TEXT("InterruptQueued %s BufferedPreserved=%s"), *FormatCommandState(CommandRequest), *FormatCommandState(BufferedCommand)));
        TryCommitPendingCommands();
        return true;
    }

    if (!ActiveActionDefinition)
    {
        LogCommandFlow(FString::Printf(TEXT("RequestRejected NoActiveDefinition Command=%s"), *FormatCommandState(CommandRequest)));
        return false;
    }

    if (ActiveActionState.NormalizedProgress < ActiveActionDefinition->QueueWindowStartsAtNormalizedTime)
    {
        LogCommandFlow(FString::Printf(
            TEXT("RequestRejected QueueNotOpen Command=%s Progress=%.2f OpensAt=%.2f"),
            *FormatCommandState(CommandRequest),
            ActiveActionState.NormalizedProgress,
            ActiveActionDefinition->QueueWindowStartsAtNormalizedTime));
        return false;
    }

    if (ActiveActionState.NormalizedProgress > ActiveActionDefinition->QueueWindowClosesAtNormalizedTime)
    {
        LogCommandFlow(FString::Printf(TEXT("RequestRejected QueueClosed Command=%s Progress=%.2f"), *FormatCommandState(CommandRequest), ActiveActionState.NormalizedProgress));
        return false;
    }

    BufferedCommand = CommandRequest;
    UpdateReplicatedStateFromLocal();
    OnBufferedCommandChanged.Broadcast(BufferedCommand.CommandTag);
    LogCommandFlow(FString::Printf(TEXT("CommandBuffered %s"), *FormatCommandState(CommandRequest)));
    return true;
}

bool UActionCombatComponent::ResolveTransitionAndStart(const FGameplayTag& FromActionTag, const FActionCombatBufferedCommandState& CommandRequest)
{
    const FActionCombatTransitionDefinition* Transition = FindTransitionInLayers(FromActionTag, CommandRequest);
    if (!Transition)
    {
        LogCommandFlow(FString::Printf(TEXT("TransitionMissing From=%s Command=%s"), *FromActionTag.ToString(), *FormatCommandState(CommandRequest)));
        return false;
    }

    const UActionCombatStyleData* SourceStyle = nullptr;
    const FActionCombatActionDefinition* ActionDefinition = FindActionDefinitionInLayers(Transition->ToActionTag, SourceStyle);
    const bool bStarted = StartActionFromDefinition(ActionDefinition, SourceStyle);
    LogCommandFlow(FString::Printf(TEXT("TransitionResolved From=%s Command=%s To=%s Result=%s"), *FromActionTag.ToString(), *FormatCommandState(CommandRequest), *Transition->ToActionTag.ToString(), bStarted ? TEXT("Started") : TEXT("Failed")));
    return bStarted;
}

bool UActionCombatComponent::StartActionFromDefinition(const FActionCombatActionDefinition* ActionDefinition, const UActionCombatStyleData* SourceStyle)
{
    if (!ActionDefinition)
    {
        return false;
    }

    FString OwnerTagFailureReason;
    if (!DoesOwnerMeetActionTagRequirements(ActionDefinition, OwnerTagFailureReason))
    {
        LogCommandFlow(FString::Printf(TEXT("ActionRejected Tag=%s Reason=%s"), *ActionDefinition->ActionTag.ToString(), *OwnerTagFailureReason));
        return false;
    }

    FString ResourceFailureReason;
    if (!TryProcessActionResourceCosts(ActionDefinition, ResourceFailureReason))
    {
        LogCommandFlow(FString::Printf(TEXT("ActionRejected Tag=%s Reason=%s"), *ActionDefinition->ActionTag.ToString(), *ResourceFailureReason));
        return false;
    }

    FActionCombatActiveActionState PreviousState = ActiveActionState;
    const bool bHadActiveAction = PreviousState.ActionTag.IsValid();

    if (bHadActiveAction)
    {
        EndActiveAction(true);
    }

    ActiveActionDefinition = ActionDefinition;
    ActiveActionSourceStyle = SourceStyle;
    ActiveActionElapsedScaledTime = 0.0f;
    ActiveActionStylePlayRateSnapshot = GetStylePlayRateSnapshot();

    ActiveActionState.Reset();
    ActiveActionState.ActionTag = ActionDefinition->ActionTag;
    ActiveActionState.ActionInstanceId = ++ActionInstanceCounter;
    ActiveActionState.EffectivePlayRate = ActionDefinition->BasePlayRate * ActiveActionStylePlayRateSnapshot * RuntimePlayRateMultiplier;
    ActiveActionState.bStartedWhileFocusActive = bFocusActive;
    ActiveActionState.TraceSourceId = ActionDefinition->TraceSourceId;
    ActiveActionState.HitWindowName = ActionDefinition->HitWindowName;
    ActiveActionState.MotionValue = ActionDefinition->MotionValue;
    ActiveActionState.PoiseDamage = ActionDefinition->PoiseDamage;
    ActiveActionState.BuildupMultiplier = ActionDefinition->BuildupMultiplier;

    if (bAutoPlayMontages && ActionDefinition->Montage)
    {
        if (USkeletalMeshComponent* MeshComponent = ResolveAnimationMesh())
        {
            if (UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance())
            {
                if (AnimInstance->Montage_Play(ActionDefinition->Montage, ActiveActionState.EffectivePlayRate) > 0.0f)
                {
                    ActiveActionState.bUsingMontageTiming = true;
                }
            }
        }
    }

    SetComponentTickEnabled(true);
    UpdateReplicatedStateFromLocal();
    OnActionStarted.Broadcast(ActiveActionState);
    LogCommandFlow(FString::Printf(TEXT("ActionStarted Tag=%s Instance=%d PlayRate=%.2f"), *ActiveActionState.ActionTag.ToString(), ActiveActionState.ActionInstanceId, ActiveActionState.EffectivePlayRate));
    return true;
}

void UActionCombatComponent::EndActiveAction(bool bWasInterrupted)
{
    if (!ActiveActionState.ActionTag.IsValid())
    {
        return;
    }

    FActionCombatActiveActionState PreviousState = ActiveActionState;
    OnActionEnded.Broadcast(PreviousState);
    LogCommandFlow(FString::Printf(TEXT("ActionEnded Tag=%s Instance=%d Interrupted=%s"), *PreviousState.ActionTag.ToString(), PreviousState.ActionInstanceId, bWasInterrupted ? TEXT("true") : TEXT("false")));

    if (bWasInterrupted && bAutoPlayMontages && ActiveActionDefinition && ActiveActionDefinition->Montage)
    {
        if (USkeletalMeshComponent* MeshComponent = ResolveAnimationMesh())
        {
            if (UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance())
            {
                AnimInstance->Montage_Stop(0.05f, ActiveActionDefinition->Montage);
            }
        }
    }

    ActiveActionState.Reset();
    ActiveActionDefinition = nullptr;
    ActiveActionSourceStyle = nullptr;
    ActiveActionElapsedScaledTime = 0.0f;
    ActiveActionStylePlayRateSnapshot = 1.0f;

    if (!BufferedCommand.IsValid() && !PendingInterruptCommand.IsValid())
    {
        SetComponentTickEnabled(false);
    }

    UpdateReplicatedStateFromLocal();
}

void UActionCombatComponent::UpdateActiveActionProgress(float DeltaTime)
{
    if (!ActiveActionDefinition)
    {
        return;
    }

    if (ActiveActionState.bUsingMontageTiming && bAutoPlayMontages && ActiveActionDefinition->Montage)
    {
        if (USkeletalMeshComponent* MeshComponent = ResolveAnimationMesh())
        {
            if (UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance())
            {
                if (AnimInstance->Montage_IsPlaying(ActiveActionDefinition->Montage))
                {
                    const float MontageLength = FMath::Max(ActiveActionDefinition->Montage->GetPlayLength(), KINDA_SMALL_NUMBER);
                    const float MontagePosition = AnimInstance->Montage_GetPosition(ActiveActionDefinition->Montage);
                    ActiveActionState.NormalizedProgress = FMath::Clamp(MontagePosition / MontageLength, 0.0f, 1.0f);
                    return;
                }
            }
        }

        ActiveActionState.NormalizedProgress = 1.0f;
        return;
    }

    const float SafeDuration = FMath::Max(ActiveActionDefinition->FallbackDurationSeconds, KINDA_SMALL_NUMBER);
    ActiveActionElapsedScaledTime += DeltaTime * ActiveActionState.EffectivePlayRate;
    ActiveActionState.NormalizedProgress = FMath::Clamp(ActiveActionElapsedScaledTime / SafeDuration, 0.0f, 1.0f);
}

void UActionCombatComponent::TryCommitPendingCommands()
{
    if (!ActiveActionDefinition || !ActiveActionState.ActionTag.IsValid())
    {
        return;
    }

    // Interrupt commands are evaluated first so dodge-cancel clears the queued follow-up.
    if (PendingInterruptCommand.IsValid()
        && ActiveActionDefinition->bAllowDodgeCancel
        && ActiveActionState.NormalizedProgress >= ActiveActionDefinition->DodgeCancelStartsAtNormalizedTime)
    {
        const FActionCombatBufferedCommandState InterruptCommand = PendingInterruptCommand;
        const FActionCombatBufferedCommandState SavedBufferedCommand = BufferedCommand;
        PendingInterruptCommand.Reset();
        BufferedCommand.Reset();

        if (ResolveTransitionAndStart(ActiveActionState.ActionTag, InterruptCommand))
        {
            if (SavedBufferedCommand.IsValid())
            {
                OnBufferedCommandChanged.Broadcast(BufferedCommand.CommandTag);
            }

            LogCommandFlow(FString::Printf(TEXT("InterruptCommitted %s"), *FormatCommandState(InterruptCommand)));
            UpdateReplicatedStateFromLocal();
            return;
        }

        BufferedCommand = SavedBufferedCommand;
        UpdateReplicatedStateFromLocal();
        LogCommandFlow(FString::Printf(TEXT("InterruptRejected %s BufferedKept=%s"), *FormatCommandState(InterruptCommand), *FormatCommandState(BufferedCommand)));
    }

    if (BufferedCommand.IsValid()
        && ActiveActionState.NormalizedProgress >= ActiveActionDefinition->ChainCommitAtNormalizedTime)
    {
        const FActionCombatBufferedCommandState QueuedCommand = BufferedCommand;
        BufferedCommand.Reset();
        OnBufferedCommandChanged.Broadcast(BufferedCommand.CommandTag);
        LogCommandFlow(FString::Printf(TEXT("BufferedCommandCommitted %s"), *FormatCommandState(QueuedCommand)));

        if (ResolveTransitionAndStart(ActiveActionState.ActionTag, QueuedCommand))
        {
            UpdateReplicatedStateFromLocal();
            return;
        }
    }

    if (IsCurrentActionFinished())
    {
        const FActionCombatBufferedCommandState QueuedCommand = BufferedCommand;
        BufferedCommand.Reset();
        PendingInterruptCommand.Reset();
        OnBufferedCommandChanged.Broadcast(BufferedCommand.CommandTag);

        if (QueuedCommand.IsValid() && ResolveTransitionAndStart(ActiveActionState.ActionTag, QueuedCommand))
        {
            UpdateReplicatedStateFromLocal();
            return;
        }

        EndActiveAction(false);
    }
}

void UActionCombatComponent::RefreshActiveMontagePlayRate()
{
    if (!bAutoPlayMontages || !ActiveActionDefinition || !ActiveActionDefinition->Montage)
    {
        return;
    }

    if (USkeletalMeshComponent* MeshComponent = ResolveAnimationMesh())
    {
        if (UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance())
        {
            AnimInstance->Montage_SetPlayRate(ActiveActionDefinition->Montage, ActiveActionState.EffectivePlayRate);
        }
    }
}

void UActionCombatComponent::SyncReplicatedMontage()
{
    USkeletalMeshComponent* MeshComponent = ResolveAnimationMesh();
    if (!MeshComponent)
    {
        return;
    }

    UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance();
    if (!AnimInstance)
    {
        return;
    }

    UAnimMontage* IncomingMontage = ReplicatedState.ActiveMontage.Get();
    if (!IncomingMontage)
    {
        if (LastReplicatedMontage.IsValid())
        {
            AnimInstance->Montage_Stop(0.05f, LastReplicatedMontage.Get());
            LastReplicatedMontage = nullptr;
            LastReplicatedMontageActionInstanceId = 0;
            LogCommandFlow(TEXT("RepMontageStopped"));
        }
        return;
    }

    const bool bActionChanged = LastReplicatedMontageActionInstanceId != ReplicatedState.ActionInstanceId || LastReplicatedMontage.Get() != IncomingMontage;
    const AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
    const float ServerNow = GameState ? GameState->GetServerWorldTimeSeconds() : ReplicatedState.MontageServerStartTimeSeconds;
    const float EstimatedElapsed = FMath::Max(ServerNow - ReplicatedState.MontageServerStartTimeSeconds, 0.0f);
    const float MontageLength = FMath::Max(IncomingMontage->GetPlayLength(), KINDA_SMALL_NUMBER);
    const float DesiredPosition = FMath::Clamp(EstimatedElapsed * ReplicatedState.EffectivePlayRate, 0.0f, MontageLength - KINDA_SMALL_NUMBER);
    ActiveActionState.NormalizedProgress = FMath::Clamp(DesiredPosition / MontageLength, 0.0f, 1.0f);

    if (bActionChanged)
    {
        AnimInstance->Montage_Play(IncomingMontage, ReplicatedState.EffectivePlayRate);
        AnimInstance->Montage_SetPosition(IncomingMontage, DesiredPosition);
        LastReplicatedMontage = IncomingMontage;
        LastReplicatedMontageActionInstanceId = ReplicatedState.ActionInstanceId;
        LogCommandFlow(FString::Printf(TEXT("RepMontageStarted Montage=%s ActionInstance=%d Pos=%.2f"), *GetNameSafe(IncomingMontage), ReplicatedState.ActionInstanceId, DesiredPosition));
        return;
    }

    AnimInstance->Montage_SetPlayRate(IncomingMontage, ReplicatedState.EffectivePlayRate);
    const float CurrentPosition = AnimInstance->Montage_GetPosition(IncomingMontage);
    if (FMath::Abs(CurrentPosition - DesiredPosition) > 0.1f)
    {
        AnimInstance->Montage_SetPosition(IncomingMontage, DesiredPosition);
    }
}

void UActionCombatComponent::SortStyleLayers()
{
    StyleLayers.Sort([](const FActionCombatStyleLayerEntry& Left, const FActionCombatStyleLayerEntry& Right)
    {
        if (Left.Priority != Right.Priority)
        {
            return Left.Priority < Right.Priority;
        }

        return Left.LayerId.LexicalLess(Right.LayerId);
    });
}

void UActionCombatComponent::UpdateReplicatedStateFromLocal()
{
    if (!HasRuntimeAuthority())
    {
        return;
    }

    ReplicatedState.ActiveActionTag = ActiveActionState.ActionTag;
    ReplicatedState.ActionInstanceId = ActiveActionState.ActionInstanceId;
    ReplicatedState.BufferedCommandTag = BufferedCommand.CommandTag;
    ReplicatedState.PendingInterruptCommandTag = PendingInterruptCommand.CommandTag;
    ReplicatedState.bFocusActive = bFocusActive;
    ReplicatedState.EffectivePlayRate = ActiveActionState.EffectivePlayRate;
    ReplicatedState.ActiveMontage = (ActiveActionDefinition != nullptr) ? ActiveActionDefinition->Montage : nullptr;

    float ElapsedUnscaledSeconds = 0.0f;
    if (ActiveActionDefinition != nullptr)
    {
        if (ActiveActionDefinition->Montage != nullptr)
        {
            if (USkeletalMeshComponent* MeshComponent = ResolveAnimationMesh())
            {
                if (UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance())
                {
                    ElapsedUnscaledSeconds = AnimInstance->Montage_GetPosition(ActiveActionDefinition->Montage);
                }
            }
        }
        else
        {
            ElapsedUnscaledSeconds = ActiveActionElapsedScaledTime / FMath::Max(ActiveActionState.EffectivePlayRate, KINDA_SMALL_NUMBER);
        }
    }

    ReplicatedState.MontageServerStartTimeSeconds = (GetWorld() != nullptr && ActiveActionDefinition != nullptr)
        ? (GetWorld()->GetTimeSeconds() - ElapsedUnscaledSeconds)
        : 0.0f;
}

float UActionCombatComponent::GetStylePlayRateSnapshot() const
{
    float Result = 1.0f;

    for (const FActionCombatStyleLayerEntry& Entry : StyleLayers)
    {
        if (Entry.bEnabled && Entry.Style)
        {
            Result *= Entry.Style->GetGlobalPlayRateMultiplier();
            Result *= Entry.LayerPlayRateMultiplier;
        }
    }

    return Result;
}

bool UActionCombatComponent::IsCurrentActionFinished() const
{
    return ActiveActionState.ActionTag.IsValid() && ActiveActionState.NormalizedProgress >= 1.0f;
}

const FActionCombatActionDefinition* UActionCombatComponent::FindActionDefinitionInLayers(const FGameplayTag& ActionTag, const UActionCombatStyleData*& OutSourceStyle) const
{
    for (int32 Index = StyleLayers.Num() - 1; Index >= 0; --Index)
    {
        const FActionCombatStyleLayerEntry& Entry = StyleLayers[Index];
        if (!Entry.bEnabled || !Entry.Style)
        {
            continue;
        }

        if (const FActionCombatActionDefinition* FoundAction = Entry.Style->FindActionDefinition(ActionTag))
        {
            OutSourceStyle = Entry.Style;
            return FoundAction;
        }
    }

    OutSourceStyle = nullptr;
    return nullptr;
}

const FActionCombatTransitionDefinition* UActionCombatComponent::FindTransitionInLayers(const FGameplayTag& FromActionTag, const FActionCombatBufferedCommandState& CommandRequest) const
{
    for (int32 Index = StyleLayers.Num() - 1; Index >= 0; --Index)
    {
        const FActionCombatStyleLayerEntry& Entry = StyleLayers[Index];
        if (!Entry.bEnabled || !Entry.Style)
        {
            continue;
        }

        if (const FActionCombatTransitionDefinition* FoundTransition = Entry.Style->FindTransitionDefinition(FromActionTag, CommandRequest.CommandTag, bFocusActive, CommandRequest.HeldInputTags))
        {
            return FoundTransition;
        }
    }

    return nullptr;
}

bool UActionCombatComponent::DoesOwnerMeetActionTagRequirements(const FActionCombatActionDefinition* ActionDefinition, FString& OutFailureReason) const
{
    OutFailureReason.Reset();

    if (!ActionDefinition)
    {
        OutFailureReason = TEXT("MissingActionDefinition");
        return false;
    }

    if (ActionDefinition->RequiredOwnerTags.IsEmpty() && ActionDefinition->BlockedOwnerTags.IsEmpty())
    {
        return true;
    }

    UAbilitySystemComponent* AbilitySystemComponent = ResolveAbilitySystemComponent();
    if (!AbilitySystemComponent)
    {
        if (!ActionDefinition->RequiredOwnerTags.IsEmpty())
        {
            OutFailureReason = TEXT("MissingAbilitySystemComponentForRequiredOwnerTags");
            return false;
        }

        return true;
    }

    FGameplayTagContainer OwnerTags;
    AbilitySystemComponent->GetOwnedGameplayTags(OwnerTags);

    if (!ActionDefinition->RequiredOwnerTags.IsEmpty() && !OwnerTags.HasAll(ActionDefinition->RequiredOwnerTags))
    {
        OutFailureReason = FString::Printf(TEXT("MissingOwnerTags Required=%s Current=%s"), *ActionDefinition->RequiredOwnerTags.ToStringSimple(), *OwnerTags.ToStringSimple());
        return false;
    }

    if (!ActionDefinition->BlockedOwnerTags.IsEmpty() && OwnerTags.HasAny(ActionDefinition->BlockedOwnerTags))
    {
        OutFailureReason = FString::Printf(TEXT("BlockedByOwnerTags Blocked=%s Current=%s"), *ActionDefinition->BlockedOwnerTags.ToStringSimple(), *OwnerTags.ToStringSimple());
        return false;
    }

    return true;
}

bool UActionCombatComponent::TryProcessActionResourceCosts(const FActionCombatActionDefinition* ActionDefinition, FString& OutFailureReason)
{
    OutFailureReason.Reset();

    if (!ActionDefinition || ActionDefinition->ResourceCosts.IsEmpty())
    {
        return true;
    }

    UAbilitySystemComponent* AbilitySystemComponent = ResolveAbilitySystemComponent();
    if (!AbilitySystemComponent)
    {
        OutFailureReason = TEXT("MissingAbilitySystemComponent");
        return false;
    }

    for (const FActionCombatAttributeCost& ResourceCost : ActionDefinition->ResourceCosts)
    {
        if ((ResourceCost.RequiredValue <= 0.0f) && (ResourceCost.ConsumeValue <= 0.0f))
        {
            continue;
        }

        if (!ResourceCost.Attribute.IsValid())
        {
            OutFailureReason = TEXT("InvalidResourceAttribute");
            return false;
        }

        if (!AbilitySystemComponent->HasAttributeSetForAttribute(ResourceCost.Attribute))
        {
            OutFailureReason = FString::Printf(TEXT("MissingAttributeSet %s"), *FormatAttributeName(ResourceCost.Attribute));
            return false;
        }

        const float CurrentValue = AbilitySystemComponent->GetNumericAttribute(ResourceCost.Attribute);
        const float RequiredToStart = FMath::Max(ResourceCost.RequiredValue, ResourceCost.bConsumeOnActionStart ? ResourceCost.ConsumeValue : 0.0f);

        if (ResourceCost.bBlockActionIfInsufficient && (CurrentValue + KINDA_SMALL_NUMBER < RequiredToStart))
        {
            OutFailureReason = FString::Printf(TEXT("Insufficient %s Current=%.2f Required=%.2f"), *FormatAttributeName(ResourceCost.Attribute), CurrentValue, RequiredToStart);
            return false;
        }
    }

    for (const FActionCombatAttributeCost& ResourceCost : ActionDefinition->ResourceCosts)
    {
        if (!ResourceCost.bConsumeOnActionStart || ResourceCost.ConsumeValue <= 0.0f || !ResourceCost.Attribute.IsValid())
        {
            continue;
        }

        AbilitySystemComponent->ApplyModToAttribute(ResourceCost.Attribute, EGameplayModOp::Additive, -ResourceCost.ConsumeValue);
        LogCommandFlow(FString::Printf(TEXT("ActionCostConsumed Tag=%s Attribute=%s Amount=%.2f"), *ActionDefinition->ActionTag.ToString(), *FormatAttributeName(ResourceCost.Attribute), ResourceCost.ConsumeValue));
    }

    return true;
}

UAbilitySystemComponent* UActionCombatComponent::ResolveAbilitySystemComponent() const
{
    return UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner());
}

FString UActionCombatComponent::FormatAttributeName(const FGameplayAttribute& Attribute)
{
    return Attribute.IsValid() ? Attribute.GetName() : TEXT("<InvalidAttribute>");
}

USkeletalMeshComponent* UActionCombatComponent::ResolveAnimationMesh() const
{
    if (AActor* Owner = GetOwner())
    {
        if (USkeletalMeshComponent* ExplicitMesh = Cast<USkeletalMeshComponent>(AnimationMeshComponent.GetComponent(Owner)))
        {
            return ExplicitMesh;
        }

        TArray<USkeletalMeshComponent*> SkeletalMeshes;
        Owner->GetComponents(SkeletalMeshes);
        if (SkeletalMeshes.Num() > 0)
        {
            return SkeletalMeshes[0];
        }
    }

    return nullptr;
}

FString UActionCombatComponent::FormatCommandState(const FActionCombatBufferedCommandState& CommandState) const
{
    if (!CommandState.IsValid())
    {
        return TEXT("<None>");
    }

    FString Result = CommandState.CommandTag.ToString();
    if (!CommandState.HeldInputTags.IsEmpty())
    {
        TArray<FGameplayTag> HeldTags;
        CommandState.HeldInputTags.GetGameplayTagArray(HeldTags);

        TArray<FString> HeldStrings;
        for (const FGameplayTag& HeldTag : HeldTags)
        {
            HeldStrings.Add(HeldTag.ToString());
        }

        Result += FString::Printf(TEXT(" Held=[%s]"), *FString::Join(HeldStrings, TEXT(", ")));
    }

    return Result;
}

FString UActionCombatComponent::BuildCommandHistoryString() const
{
    return FString::Join(RecentCommandHistory, TEXT(" -> "));
}

void UActionCombatComponent::AppendCommandHistory(const FActionCombatBufferedCommandState& CommandState)
{
    if (!CommandState.IsValid())
    {
        return;
    }

    RecentCommandHistory.Add(FormatCommandState(CommandState));
    while (RecentCommandHistory.Num() > MaxCommandHistoryEntries)
    {
        RecentCommandHistory.RemoveAt(0);
    }
}

void UActionCombatComponent::LogCommandFlow(const FString& Message) const
{
    if (!bAlwaysLogCommandFlow)
    {
        return;
    }

    UE_LOG(LogActionCombatRuntime, Log, TEXT("[%s] %s"), *GetPathNameSafe(GetOwner()), *Message);
}
