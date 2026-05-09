#include "ActionCombatComponent.h"

#include "ActionCombatRuntimeLog.h"
#include "ActionCombatRuntimeTags.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameStateBase.h"
#include "Net/UnrealNetwork.h"
#include "UObject/UnrealType.h"

namespace ActionCombatComponent
{
    static const FName BaseStyleLayerId(TEXT("BaseStyle"));
    static const FName DefaultMontageSlotName(TEXT("DefaultSlot"));
    static const FName FullBodyMontageSlotName(TEXT("FullBody"));
    static const FName DisableLegIKCurveName(TEXT("DisableLegIK"));
    static const FName UseFootPlacementPropertyName(TEXT("UseFootPlacement"));

    static void PrepareAnimInstanceForMontageRootMotion(UAnimInstance* AnimInstance)
    {
        if (AnimInstance)
        {
            AnimInstance->SetRootMotionMode(ERootMotionMode::IgnoreRootMotion);
        }
    }

    static void RestoreAnimInstanceRootMotionMode(UAnimInstance* AnimInstance)
    {
        if (AnimInstance)
        {
            AnimInstance->SetRootMotionMode(ERootMotionMode::RootMotionFromMontagesOnly);
        }
    }
}

UActionCombatComponent::UActionCombatComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    PrimaryComponentTick.TickGroup = TG_PrePhysics;
    SetIsReplicatedByDefault(true);
}

void UActionCombatComponent::BeginPlay()
{
    Super::BeginPlay();

    SetComponentTickEnabled(false);
    ConfigureAnimationTickPrerequisite();
    SortStyleLayers();
    UpdateReplicatedStateFromLocal();
}

void UActionCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!HasRuntimeAuthority())
    {
        if (!ActiveActionState.ActionTag.IsValid())
        {
            SetComponentTickEnabled(false);
            return;
        }

        ApplyActiveActionAnimationOverrides();
        return;
    }

    if (!ActiveActionState.ActionTag.IsValid() || !ActiveActionDefinition)
    {
        SetComponentTickEnabled(false);
        return;
    }

    UpdateActiveActionProgress(DeltaTime);
    ApplyActiveActionAnimationOverrides();
    TickManualMontageRootMotion();
    TickAttackAdvance(DeltaTime);
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

void UActionCombatComponent::BroadcastReactionCueForActor(AActor* ReactionActor, EActionCombatReactionState NewState, FVector_NetQuantizeNormal WorldSpaceImpulseDirection, FVector_NetQuantize WorldSpaceActorLocation, int32 CueId)
{
    const AActor* Owner = GetOwner();
    if ((Owner == nullptr) || !Owner->HasAuthority() || (ReactionActor == nullptr))
    {
        return;
    }

    MulticastPlayReactionCueForActor(ReactionActor, NewState, WorldSpaceImpulseDirection, WorldSpaceActorLocation, CueId);
}

void UActionCombatComponent::MulticastPlayReactionCueForActor_Implementation(AActor* ReactionActor, EActionCombatReactionState NewState, FVector_NetQuantizeNormal WorldSpaceImpulseDirection, FVector_NetQuantize WorldSpaceActorLocation, int32 CueId)
{
    UActionCombatReactionComponent::PlayReactionCueOnActor(ReactionActor, NewState, WorldSpaceImpulseDirection, WorldSpaceActorLocation, CueId);
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
        ResetActionAnimationOverrides();
    }
    else
    {
        if ((PreviousState.ActionInstanceId != ReplicatedState.ActionInstanceId) || (PreviousState.ActionTag != ReplicatedState.ActiveActionTag))
        {
            ActiveActionState.Reset();
        }

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
        SetComponentTickEnabled(ActiveActionState.ActionTag.IsValid());
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
    InitializeAttackAdvanceForAction();

    if (bAutoPlayMontages && ActionDefinition->Montage)
    {
        if (USkeletalMeshComponent* MeshComponent = ResolveAnimationMesh())
        {
            if (UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance())
            {
                if (UAnimMontage* PlayableMontage = ResolvePlayableMontage(ActionDefinition->Montage))
                {
                    ActionCombatComponent::PrepareAnimInstanceForMontageRootMotion(AnimInstance);
                    ApplyActiveActionAnimationOverrides();

                    if (AnimInstance->Montage_Play(PlayableMontage, ActiveActionState.EffectivePlayRate) > 0.0f)
                    {
                        ActiveActionState.bUsingMontageTiming = true;
                        InitializeManualMontageRootMotion(PlayableMontage, AnimInstance);
                    }
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
                if (UAnimMontage* PlayableMontage = ResolvePlayableMontage(ActiveActionDefinition->Montage))
                {
                    AnimInstance->Montage_Stop(0.05f, PlayableMontage);
                }
            }
        }
    }

    ActiveActionState.Reset();
    ActiveActionDefinition = nullptr;
    ActiveActionSourceStyle = nullptr;
    ActiveActionElapsedScaledTime = 0.0f;
    ActiveActionStylePlayRateSnapshot = 1.0f;
    ResetManualMontageRootMotion();
    ResetActionAnimationOverrides();

    if (!BufferedCommand.IsValid() && !PendingInterruptCommand.IsValid())
    {
        SetComponentTickEnabled(false);
    }

    UpdateReplicatedStateFromLocal();
}

void UActionCombatComponent::InitializeAttackAdvanceForAction()
{
    ActiveActionState.AttackAdvanceAppliedDistance = 0.0f;
    ActiveActionState.AttackAdvanceDirection = FVector::ZeroVector;
    ActiveActionState.bAttackAdvanceBlocked = false;
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
                if (UAnimMontage* PlayableMontage = ResolvePlayableMontage(ActiveActionDefinition->Montage))
                {
                    if (AnimInstance->Montage_IsPlaying(PlayableMontage))
                    {
                        const float MontageLength = FMath::Max(PlayableMontage->GetPlayLength(), KINDA_SMALL_NUMBER);
                        const float MontagePosition = AnimInstance->Montage_GetPosition(PlayableMontage);
                        ActiveActionState.NormalizedProgress = FMath::Clamp(MontagePosition / MontageLength, 0.0f, 1.0f);
                        return;
                    }
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

void UActionCombatComponent::InitializeManualMontageRootMotion(UAnimMontage* PlayableMontage, UAnimInstance* AnimInstance)
{
    ResetManualMontageRootMotion();

    if (!AnimInstance || !ShouldManuallyDriveMontageRootMotion(PlayableMontage))
    {
        return;
    }

    ManualRootMotionMontage = PlayableMontage;
    ManualRootMotionPreviousPosition = AnimInstance->Montage_GetPosition(PlayableMontage);
    ManualRootMotionActionInstanceId = ActiveActionState.ActionInstanceId;
}

void UActionCombatComponent::TickManualMontageRootMotion()
{
    AActor* Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority() || !ActiveActionState.ActionTag.IsValid() || !ManualRootMotionMontage.IsValid())
    {
        return;
    }

    if (ManualRootMotionActionInstanceId != ActiveActionState.ActionInstanceId)
    {
        ResetManualMontageRootMotion();
        return;
    }

    USkeletalMeshComponent* MeshComponent = ResolveAnimationMesh();
    if (!MeshComponent)
    {
        return;
    }

    UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance();
    UAnimMontage* PlayableMontage = ManualRootMotionMontage.Get();
    if (!AnimInstance || !PlayableMontage || !AnimInstance->Montage_IsPlaying(PlayableMontage))
    {
        return;
    }

    const float CurrentPosition = AnimInstance->Montage_GetPosition(PlayableMontage);
    const float PreviousPosition = ManualRootMotionPreviousPosition;
    ManualRootMotionPreviousPosition = CurrentPosition;

    if (FMath::IsNearlyEqual(CurrentPosition, PreviousPosition))
    {
        return;
    }

    const FTransform LocalRootMotion = PlayableMontage->ExtractRootMotionFromTrackRange(PreviousPosition, CurrentPosition);
    if (LocalRootMotion.GetTranslation().IsNearlyZero() && LocalRootMotion.GetRotation().IsIdentity())
    {
        return;
    }

    const FTransform WorldRootMotion = MeshComponent->ConvertLocalRootMotionToWorld(LocalRootMotion);
    const FVector WorldDelta = WorldRootMotion.GetTranslation();
    if (WorldDelta.IsNearlyZero())
    {
        return;
    }

    const FVector PreviousLocation = Owner->GetActorLocation();
    FHitResult MoveHit;
    if (!MoveOwnerForAttackAdvance(WorldDelta, MoveHit))
    {
        return;
    }

    const bool bMoved = !Owner->GetActorLocation().Equals(PreviousLocation, KINDA_SMALL_NUMBER);
    if (bMoved || MoveHit.IsValidBlockingHit())
    {
        Owner->ForceNetUpdate();
    }
}

void UActionCombatComponent::ResetManualMontageRootMotion()
{
    ManualRootMotionMontage = nullptr;
    ManualRootMotionPreviousPosition = 0.0f;
    ManualRootMotionActionInstanceId = 0;
}

bool UActionCombatComponent::ShouldManuallyDriveMontageRootMotion(const UAnimMontage* PlayableMontage) const
{
    const AActor* Owner = GetOwner();
    return Owner && Owner->HasAuthority() && PlayableMontage && PlayableMontage->HasRootMotion();
}

void UActionCombatComponent::ConfigureAnimationTickPrerequisite()
{
    if (USkeletalMeshComponent* MeshComponent = ResolveAnimationMesh())
    {
        MeshComponent->AddTickPrerequisiteComponent(this);
    }
}

void UActionCombatComponent::ApplyActiveActionAnimationOverrides()
{
    if (!ActiveActionState.ActionTag.IsValid())
    {
        return;
    }

    USkeletalMeshComponent* MeshComponent = ResolveAnimationMesh();
    if (!MeshComponent)
    {
        return;
    }

    if (UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance())
    {
        ApplyFootPlacementOverride(AnimInstance, true);
    }

    const USkeletalMeshComponent* ConstMeshComponent = MeshComponent;
    for (UAnimInstance* LinkedAnimInstance : ConstMeshComponent->GetLinkedAnimInstances())
    {
        ApplyFootPlacementOverride(LinkedAnimInstance, true);
    }
}

void UActionCombatComponent::ResetActionAnimationOverrides()
{
    if (USkeletalMeshComponent* MeshComponent = ResolveAnimationMesh())
    {
        if (UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance())
        {
            ApplyFootPlacementOverride(AnimInstance, false);
            ActionCombatComponent::RestoreAnimInstanceRootMotionMode(AnimInstance);
        }

        const USkeletalMeshComponent* ConstMeshComponent = MeshComponent;
        for (UAnimInstance* LinkedAnimInstance : ConstMeshComponent->GetLinkedAnimInstances())
        {
            ApplyFootPlacementOverride(LinkedAnimInstance, false);
        }
    }
}

void UActionCombatComponent::ApplyFootPlacementOverride(UAnimInstance* AnimInstance, bool bDisableFootPlacement)
{
    if (!AnimInstance)
    {
        return;
    }

    const float DisableLegIKValue = bDisableFootPlacement ? 1.0f : 0.0f;
    AnimInstance->AddCurveValue(ActionCombatComponent::DisableLegIKCurveName, DisableLegIKValue);
    AnimInstance->OverrideCurveValue(ActionCombatComponent::DisableLegIKCurveName, DisableLegIKValue);
    SetAnimBoolProperty(AnimInstance, ActionCombatComponent::UseFootPlacementPropertyName, !bDisableFootPlacement);
    SetAnimFloatProperty(AnimInstance, ActionCombatComponent::DisableLegIKCurveName, DisableLegIKValue);
}

bool UActionCombatComponent::SetAnimBoolProperty(UAnimInstance* AnimInstance, FName PropertyName, bool bValue)
{
    if (!AnimInstance || PropertyName.IsNone())
    {
        return false;
    }

    if (FBoolProperty* BoolProperty = FindFProperty<FBoolProperty>(AnimInstance->GetClass(), PropertyName))
    {
        BoolProperty->SetPropertyValue_InContainer(AnimInstance, bValue);
        return true;
    }

    return false;
}

bool UActionCombatComponent::SetAnimFloatProperty(UAnimInstance* AnimInstance, FName PropertyName, float Value)
{
    if (!AnimInstance || PropertyName.IsNone())
    {
        return false;
    }

    if (FFloatProperty* FloatProperty = FindFProperty<FFloatProperty>(AnimInstance->GetClass(), PropertyName))
    {
        FloatProperty->SetPropertyValue_InContainer(AnimInstance, Value);
        return true;
    }

    if (FDoubleProperty* DoubleProperty = FindFProperty<FDoubleProperty>(AnimInstance->GetClass(), PropertyName))
    {
        DoubleProperty->SetPropertyValue_InContainer(AnimInstance, static_cast<double>(Value));
        return true;
    }

    return false;
}

void UActionCombatComponent::TickAttackAdvance(float DeltaTime)
{
    (void)DeltaTime;

    AActor* Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority() || !ActiveActionDefinition || !ActiveActionState.ActionTag.IsValid())
    {
        return;
    }

    const FActionCombatAttackAdvanceSettings& AttackAdvance = ActiveActionDefinition->AttackAdvance;
    if (!AttackAdvance.bEnabled || AttackAdvance.Distance <= 0.0f || ActiveActionState.bAttackAdvanceBlocked)
    {
        return;
    }

    if (!CanApplyAttackAdvance())
    {
        return;
    }

    if (bAutoPlayMontages && ActiveActionDefinition->Montage)
    {
        if (UAnimMontage* PlayableMontage = ResolvePlayableMontage(ActiveActionDefinition->Montage))
        {
            if (PlayableMontage->HasRootMotion())
            {
                return;
            }
        }
    }

    const float StartTime = FMath::Clamp(AttackAdvance.StartNormalizedTime, 0.0f, 1.0f);
    const float EndTime = FMath::Clamp(AttackAdvance.EndNormalizedTime, 0.0f, 1.0f);
    if (EndTime <= StartTime + KINDA_SMALL_NUMBER)
    {
        return;
    }

    if (ActiveActionState.NormalizedProgress <= StartTime)
    {
        return;
    }

    if (ActiveActionState.AttackAdvanceDirection.IsNearlyZero())
    {
        ActiveActionState.AttackAdvanceDirection = ResolveAttackAdvanceDirection();
    }

    if (ActiveActionState.AttackAdvanceDirection.IsNearlyZero())
    {
        return;
    }

    const float WindowAlpha = FMath::Clamp((ActiveActionState.NormalizedProgress - StartTime) / (EndTime - StartTime), 0.0f, 1.0f);
    const float CurvedAlpha = FMath::Pow(WindowAlpha, FMath::Max(AttackAdvance.CurveExponent, 0.1f));
    const float DesiredDistance = AttackAdvance.Distance * CurvedAlpha;
    const float DeltaDistance = DesiredDistance - ActiveActionState.AttackAdvanceAppliedDistance;
    if (DeltaDistance <= KINDA_SMALL_NUMBER)
    {
        return;
    }

    const FVector PreviousLocation = Owner->GetActorLocation();
    FHitResult MoveHit;
    if (!MoveOwnerForAttackAdvance(ActiveActionState.AttackAdvanceDirection * DeltaDistance, MoveHit))
    {
        return;
    }

    const FVector ActualDelta = Owner->GetActorLocation() - PreviousLocation;
    const float ActualDistance = FVector::DotProduct(ActualDelta, ActiveActionState.AttackAdvanceDirection);
    ActiveActionState.AttackAdvanceAppliedDistance += FMath::Max(ActualDistance, 0.0f);

    if (AttackAdvance.bStopOnBlockingHit && MoveHit.IsValidBlockingHit())
    {
        ActiveActionState.bAttackAdvanceBlocked = true;
    }

    if ((ActualDistance > KINDA_SMALL_NUMBER) || MoveHit.IsValidBlockingHit())
    {
        Owner->ForceNetUpdate();
    }
}

bool UActionCombatComponent::CanApplyAttackAdvance() const
{
    const AActor* Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority() || !ActiveActionDefinition)
    {
        return false;
    }

    const FActionCombatAttackAdvanceSettings& AttackAdvance = ActiveActionDefinition->AttackAdvance;
    if (!AttackAdvance.bRequireGrounded)
    {
        return true;
    }

    const ACharacter* Character = Cast<ACharacter>(Owner);
    if (!Character)
    {
        return true;
    }

    const UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement();
    if (!MoveComp)
    {
        return false;
    }

    return MoveComp->MovementMode == MOVE_Walking || MoveComp->MovementMode == MOVE_NavWalking;
}

FVector UActionCombatComponent::ResolveAttackAdvanceDirection() const
{
    const AActor* Owner = GetOwner();
    if (!Owner)
    {
        return FVector::ZeroVector;
    }

    FVector Direction = Owner->GetActorForwardVector();
    Direction.Z = 0.0f;
    return Direction.GetSafeNormal();
}

bool UActionCombatComponent::MoveOwnerForAttackAdvance(const FVector& Delta, FHitResult& OutHit)
{
    AActor* Owner = GetOwner();
    if (!Owner || Delta.IsNearlyZero())
    {
        return false;
    }

    if (ACharacter* Character = Cast<ACharacter>(Owner))
    {
        if (UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement())
        {
            if (MoveComp->UpdatedComponent)
            {
                MoveComp->SafeMoveUpdatedComponent(Delta, Character->GetActorQuat(), true, OutHit, ETeleportType::None);
                return true;
            }
        }
    }

    Owner->AddActorWorldOffset(Delta, true, &OutHit, ETeleportType::None);
    return true;
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
            if (UAnimMontage* PlayableMontage = ResolvePlayableMontage(ActiveActionDefinition->Montage))
            {
                AnimInstance->Montage_SetPlayRate(PlayableMontage, ActiveActionState.EffectivePlayRate);
            }
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

    UAnimMontage* PlayableMontage = ResolvePlayableMontage(IncomingMontage);
    if (!PlayableMontage)
    {
        return;
    }

    const bool bActionChanged = LastReplicatedMontageActionInstanceId != ReplicatedState.ActionInstanceId || LastReplicatedMontage.Get() != PlayableMontage;
    const AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
    const float ServerNow = GameState ? GameState->GetServerWorldTimeSeconds() : ReplicatedState.MontageServerStartTimeSeconds;
    const float EstimatedElapsed = FMath::Max(ServerNow - ReplicatedState.MontageServerStartTimeSeconds, 0.0f);
    const float MontageLength = FMath::Max(PlayableMontage->GetPlayLength(), KINDA_SMALL_NUMBER);
    const float DesiredPosition = FMath::Clamp(EstimatedElapsed * ReplicatedState.EffectivePlayRate, 0.0f, MontageLength - KINDA_SMALL_NUMBER);
    ActiveActionState.NormalizedProgress = FMath::Clamp(DesiredPosition / MontageLength, 0.0f, 1.0f);

    if (bActionChanged)
    {
        ActionCombatComponent::PrepareAnimInstanceForMontageRootMotion(AnimInstance);
        AnimInstance->Montage_Play(PlayableMontage, ReplicatedState.EffectivePlayRate);
        AnimInstance->Montage_SetPosition(PlayableMontage, DesiredPosition);
        LastReplicatedMontage = PlayableMontage;
        LastReplicatedMontageActionInstanceId = ReplicatedState.ActionInstanceId;
        LogCommandFlow(FString::Printf(TEXT("RepMontageStarted Montage=%s ActionInstance=%d Pos=%.2f"), *GetNameSafe(PlayableMontage), ReplicatedState.ActionInstanceId, DesiredPosition));
        return;
    }

    AnimInstance->Montage_SetPlayRate(PlayableMontage, ReplicatedState.EffectivePlayRate);
    const float CurrentPosition = AnimInstance->Montage_GetPosition(PlayableMontage);
    if (FMath::Abs(CurrentPosition - DesiredPosition) > 0.1f)
    {
        AnimInstance->Montage_SetPosition(PlayableMontage, DesiredPosition);
    }
}

bool UActionCombatComponent::ShouldUseFullBodyMontageOverride(const UAnimMontage* SourceMontage)
{
    return SourceMontage
        && SourceMontage->SlotAnimTracks.Num() == 1
        && SourceMontage->SlotAnimTracks[0].SlotName == ActionCombatComponent::DefaultMontageSlotName
        && SourceMontage->GetFirstAnimReference() != nullptr;
}

UAnimMontage* UActionCombatComponent::ResolvePlayableMontage(UAnimMontage* SourceMontage)
{
    if (!ShouldUseFullBodyMontageOverride(SourceMontage))
    {
        return SourceMontage;
    }

    if (TObjectPtr<UAnimMontage>* ExistingOverride = RuntimeMontageOverrides.Find(SourceMontage))
    {
        return ExistingOverride->Get();
    }

    UAnimSequenceBase* SourceSequence = SourceMontage->GetFirstAnimReference();
    if (!SourceSequence)
    {
        return SourceMontage;
    }

    UAnimMontage* OverrideMontage = UAnimMontage::CreateSlotAnimationAsDynamicMontage(
        SourceSequence,
        ActionCombatComponent::FullBodyMontageSlotName,
        SourceMontage->GetDefaultBlendInTime(),
        SourceMontage->GetDefaultBlendOutTime(),
        1.0f,
        1,
        SourceMontage->BlendOutTriggerTime,
        0.0f);

    if (!OverrideMontage)
    {
        return SourceMontage;
    }

    OverrideMontage->bEnableAutoBlendOut = SourceMontage->bEnableAutoBlendOut;
    RuntimeMontageOverrides.Add(SourceMontage, OverrideMontage);
    return OverrideMontage;
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
                    if (UAnimMontage* PlayableMontage = ResolvePlayableMontage(ActiveActionDefinition->Montage))
                    {
                        ElapsedUnscaledSeconds = AnimInstance->Montage_GetPosition(PlayableMontage);
                    }
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
        UAbilitySystemComponent* AbilitySystemComponent = ResolveAbilitySystemComponent();
        if (!AbilitySystemComponent)
        {
            return true;
        }

        FGameplayTagContainer OwnerTags;
        AbilitySystemComponent->GetOwnedGameplayTags(OwnerTags);
        if (OwnerTags.HasTag(ActionCombatRuntimeTags::Combat_State_Reaction))
        {
            OutFailureReason = FString::Printf(TEXT("BlockedByReactionState Current=%s"), *OwnerTags.ToStringSimple());
            return false;
        }

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

    if (OwnerTags.HasTag(ActionCombatRuntimeTags::Combat_State_Reaction))
    {
        OutFailureReason = FString::Printf(TEXT("BlockedByReactionState Current=%s"), *OwnerTags.ToStringSimple());
        return false;
    }

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
