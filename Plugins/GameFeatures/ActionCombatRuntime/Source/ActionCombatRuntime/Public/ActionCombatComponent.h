#pragma once

#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "GameplayTagContainer.h"

#include "ActionCombatReactionComponent.h"
#include "ActionCombatStyleData.h"
#include "ActionCombatComponent.generated.h"

class AActor;
class UAnimInstance;
class UAnimMontage;
class UAbilitySystemComponent;
class USkeletalMeshComponent;

USTRUCT(BlueprintType)
struct ACTIONCOMBATRUNTIME_API FActionCombatStyleLayerEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Style")
    FName LayerId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Style")
    int32 Priority = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Style")
    TObjectPtr<UActionCombatStyleData> Style = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Style", meta = (ClampMin = "0.01"))
    float LayerPlayRateMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Style")
    bool bEnabled = true;
};

USTRUCT(BlueprintType)
struct ACTIONCOMBATRUNTIME_API FActionCombatActiveActionState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Action")
    FGameplayTag ActionTag;

    UPROPERTY(BlueprintReadOnly, Category = "Action")
    int32 ActionInstanceId = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Action")
    float NormalizedProgress = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Action")
    float EffectivePlayRate = 1.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Action")
    bool bStartedWhileFocusActive = false;

    UPROPERTY(BlueprintReadOnly, Category = "Action")
    bool bUsingMontageTiming = false;

    UPROPERTY(BlueprintReadOnly, Category = "Action")
    FName TraceSourceId = NAME_None;

    UPROPERTY(BlueprintReadOnly, Category = "Action")
    FName HitWindowName = NAME_None;

    UPROPERTY(BlueprintReadOnly, Category = "Damage")
    float MotionValue = 1.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Damage")
    float PoiseDamage = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Damage")
    float BuildupMultiplier = 1.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Action")
    float AttackAdvanceAppliedDistance = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Action")
    FVector AttackAdvanceDirection = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category = "Action")
    bool bAttackAdvanceBlocked = false;

    void Reset()
    {
        ActionTag = FGameplayTag();
        ActionInstanceId = 0;
        NormalizedProgress = 0.0f;
        EffectivePlayRate = 1.0f;
        bStartedWhileFocusActive = false;
        bUsingMontageTiming = false;
        TraceSourceId = NAME_None;
        HitWindowName = NAME_None;
        MotionValue = 1.0f;
        PoiseDamage = 0.0f;
        BuildupMultiplier = 1.0f;
        AttackAdvanceAppliedDistance = 0.0f;
        AttackAdvanceDirection = FVector::ZeroVector;
        bAttackAdvanceBlocked = false;
    }
};

USTRUCT()
struct ACTIONCOMBATRUNTIME_API FActionCombatReplicatedState
{
    GENERATED_BODY()

    UPROPERTY()
    FGameplayTag ActiveActionTag;

    UPROPERTY()
    int32 ActionInstanceId = 0;

    UPROPERTY()
    FGameplayTag BufferedCommandTag;

    UPROPERTY()
    FGameplayTag PendingInterruptCommandTag;

    UPROPERTY()
    bool bFocusActive = false;

    UPROPERTY()
    float EffectivePlayRate = 1.0f;

    UPROPERTY()
    TObjectPtr<UAnimMontage> ActiveMontage = nullptr;

    UPROPERTY()
    float MontageServerStartTimeSeconds = 0.0f;
};

USTRUCT(BlueprintType)
struct ACTIONCOMBATRUNTIME_API FActionCombatBufferedCommandState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Command")
    FGameplayTag CommandTag;

    UPROPERTY(BlueprintReadOnly, Category = "Command")
    FGameplayTagContainer HeldInputTags;

    bool IsValid() const
    {
        return CommandTag.IsValid();
    }

    void Reset()
    {
        CommandTag = FGameplayTag();
        HeldInputTags.Reset();
    }
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FActionCombatActionStateSignature, FActionCombatActiveActionState, ActionState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FActionCombatCommandSignature, FGameplayTag, CommandTag);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FActionCombatFocusSignature, bool, bIsFocusActive);

UCLASS(BlueprintType, ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class ACTIONCOMBATRUNTIME_API UActionCombatComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UActionCombatComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(BlueprintCallable, Category = "Action Combat|Style")
    void SetBaseStyle(UActionCombatStyleData* StyleData);

    UFUNCTION(BlueprintCallable, Category = "Action Combat|Style")
    void SetStyleLayer(FName LayerId, UActionCombatStyleData* StyleData, int32 Priority = 0, float LayerPlayRateMultiplier = 1.0f);

    UFUNCTION(BlueprintCallable, Category = "Action Combat|Style")
    void ClearStyleLayer(FName LayerId);

    UFUNCTION(BlueprintCallable, Category = "Action Combat|Style")
    void ClearAllStyleLayers();

    UFUNCTION(BlueprintPure, Category = "Action Combat|Style")
    TArray<FActionCombatStyleLayerEntry> GetStyleLayers() const
    {
        return StyleLayers;
    }

    UFUNCTION(BlueprintCallable, Category = "Action Combat|Runtime")
    void SetRuntimePlayRateMultiplier(float NewMultiplier);

    UFUNCTION(BlueprintPure, Category = "Action Combat|Runtime")
    float GetRuntimePlayRateMultiplier() const
    {
        return RuntimePlayRateMultiplier;
    }

    UFUNCTION(BlueprintCallable, Category = "Action Combat|Runtime")
    bool RequestCommand(FGameplayTag CommandTag);

    UFUNCTION(BlueprintCallable, Category = "Action Combat|Runtime")
    bool RequestCommandWithHeldTags(FGameplayTag CommandTag, FGameplayTagContainer HeldInputTagsSnapshot);

    UFUNCTION(BlueprintCallable, Category = "Action Combat|Runtime")
    void SetInputStateTagActive(FGameplayTag InputStateTag, bool bNewActiveState);

    UFUNCTION(BlueprintCallable, Category = "Action Combat|Runtime")
    bool SetFocusActive(bool bNewFocusActive);

    UFUNCTION(BlueprintPure, Category = "Action Combat|Runtime")
    bool IsFocusActive() const
    {
        return bFocusActive;
    }

    UFUNCTION(BlueprintPure, Category = "Action Combat|Runtime")
    FActionCombatActiveActionState GetActiveActionState() const
    {
        return ActiveActionState;
    }

    UFUNCTION(BlueprintPure, Category = "Action Combat|Runtime")
    FGameplayTag GetBufferedCommandTag() const
    {
        return BufferedCommand.CommandTag;
    }

    UFUNCTION(BlueprintPure, Category = "Action Combat|Runtime")
    FGameplayTag GetPendingInterruptCommandTag() const
    {
        return PendingInterruptCommand.CommandTag;
    }

    UFUNCTION(BlueprintPure, Category = "Action Combat|Runtime")
    FGameplayTagContainer GetHeldInputTags() const
    {
        return HeldInputTags;
    }

    UFUNCTION(BlueprintCallable, Category = "Action Combat|Runtime")
    void ClearPendingCommands();

    UFUNCTION(BlueprintCallable, Category = "Action Combat|Runtime")
    bool TryForceCommitBufferedCommand();

    UFUNCTION(BlueprintCallable, Category = "Action Combat|Runtime")
    void InterruptActiveAction();

    void BroadcastReactionCueForActor(AActor* ReactionActor, EActionCombatReactionState NewState, FVector_NetQuantizeNormal WorldSpaceImpulseDirection, FVector_NetQuantize WorldSpaceActorLocation, int32 CueId);

    UFUNCTION(BlueprintCallable, Category = "Action Combat|Debug")
    bool StartActionFromTag(FGameplayTag ActionTag);

    UPROPERTY(BlueprintAssignable, Category = "Action Combat|Events")
    FActionCombatActionStateSignature OnActionStarted;

    UPROPERTY(BlueprintAssignable, Category = "Action Combat|Events")
    FActionCombatActionStateSignature OnActionEnded;

    UPROPERTY(BlueprintAssignable, Category = "Action Combat|Events")
    FActionCombatCommandSignature OnBufferedCommandChanged;

    UPROPERTY(BlueprintAssignable, Category = "Action Combat|Events")
    FActionCombatFocusSignature OnFocusChanged;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
    FComponentReference AnimationMeshComponent;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
    bool bAutoPlayMontages = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Networking")
    bool bAuthorityOnly = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Debug")
    bool bAlwaysLogCommandFlow = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Debug", meta = (ClampMin = "1"))
    int32 MaxCommandHistoryEntries = 8;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Commands")
    FGameplayTag DodgeCommandTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Runtime", meta = (ClampMin = "0.01"))
    float RuntimePlayRateMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Style")
    TArray<FActionCombatStyleLayerEntry> StyleLayers;

    UPROPERTY(ReplicatedUsing = OnRep_ReplicatedState)
    FActionCombatReplicatedState ReplicatedState;

private:
    UFUNCTION(Server, Reliable)
    void ServerRequestCommand(FGameplayTag CommandTag, FGameplayTagContainer HeldInputTagsSnapshot);

    UFUNCTION(Server, Reliable)
    void ServerSetFocusActive(bool bNewFocusActive);

    UFUNCTION()
    void OnRep_ReplicatedState();

    UFUNCTION(NetMulticast, Reliable)
    void MulticastPlayReactionCueForActor(AActor* ReactionActor, EActionCombatReactionState NewState, FVector_NetQuantizeNormal WorldSpaceImpulseDirection, FVector_NetQuantize WorldSpaceActorLocation, int32 CueId);

    bool HasRuntimeAuthority() const;
    bool HandleCommandRequestInternal(const FActionCombatBufferedCommandState& CommandRequest);
    bool ResolveTransitionAndStart(const FGameplayTag& FromActionTag, const FActionCombatBufferedCommandState& CommandRequest);
    bool StartActionFromDefinition(const FActionCombatActionDefinition* ActionDefinition, const UActionCombatStyleData* SourceStyle);
    void EndActiveAction(bool bWasInterrupted);
    void InitializeAttackAdvanceForAction();
    void UpdateActiveActionProgress(float DeltaTime);
    void InitializeManualMontageRootMotion(UAnimMontage* PlayableMontage, UAnimInstance* AnimInstance);
    void TickManualMontageRootMotion();
    void ResetManualMontageRootMotion();
    bool ShouldManuallyDriveMontageRootMotion(const UAnimMontage* PlayableMontage) const;
    void ConfigureAnimationTickPrerequisite();
    void ApplyActiveActionAnimationOverrides();
    void ResetActionAnimationOverrides();
    static void ApplyFootPlacementOverride(UAnimInstance* AnimInstance, bool bDisableFootPlacement);
    static bool SetAnimBoolProperty(UAnimInstance* AnimInstance, FName PropertyName, bool bValue);
    static bool SetAnimFloatProperty(UAnimInstance* AnimInstance, FName PropertyName, float Value);
    void TickAttackAdvance(float DeltaTime);
    bool CanApplyAttackAdvance() const;
    FVector ResolveAttackAdvanceDirection() const;
    bool MoveOwnerForAttackAdvance(const FVector& Delta, FHitResult& OutHit);
    void TryCommitPendingCommands();
    void RefreshActiveMontagePlayRate();
    void SyncReplicatedMontage();
    void SortStyleLayers();
    void UpdateReplicatedStateFromLocal();
    float GetStylePlayRateSnapshot() const;
    bool IsCurrentActionFinished() const;
    UAnimMontage* ResolvePlayableMontage(UAnimMontage* SourceMontage);
    static bool ShouldUseFullBodyMontageOverride(const UAnimMontage* SourceMontage);
    const FActionCombatActionDefinition* FindActionDefinitionInLayers(const FGameplayTag& ActionTag, const UActionCombatStyleData*& OutSourceStyle) const;
    const FActionCombatTransitionDefinition* FindTransitionInLayers(const FGameplayTag& FromActionTag, const FActionCombatBufferedCommandState& CommandRequest) const;
    bool DoesOwnerMeetActionTagRequirements(const FActionCombatActionDefinition* ActionDefinition, FString& OutFailureReason) const;
    bool TryProcessActionResourceCosts(const FActionCombatActionDefinition* ActionDefinition, FString& OutFailureReason);
    UAbilitySystemComponent* ResolveAbilitySystemComponent() const;
    static FString FormatAttributeName(const FGameplayAttribute& Attribute);
    USkeletalMeshComponent* ResolveAnimationMesh() const;
    FString FormatCommandState(const FActionCombatBufferedCommandState& CommandState) const;
    FString BuildCommandHistoryString() const;
    void AppendCommandHistory(const FActionCombatBufferedCommandState& CommandState);
    void LogCommandFlow(const FString& Message) const;

    FActionCombatActiveActionState ActiveActionState;
    TWeakObjectPtr<const UActionCombatStyleData> ActiveActionSourceStyle;
    const FActionCombatActionDefinition* ActiveActionDefinition = nullptr;
    float ActiveActionElapsedScaledTime = 0.0f;
    float ActiveActionStylePlayRateSnapshot = 1.0f;
    int32 ActionInstanceCounter = 0;
    TWeakObjectPtr<UAnimMontage> ManualRootMotionMontage;
    float ManualRootMotionPreviousPosition = 0.0f;
    int32 ManualRootMotionActionInstanceId = 0;
    FActionCombatBufferedCommandState BufferedCommand;
    FActionCombatBufferedCommandState PendingInterruptCommand;
    FGameplayTagContainer HeldInputTags;
    TArray<FString> RecentCommandHistory;
    TWeakObjectPtr<UAnimMontage> LastReplicatedMontage;
    int32 LastReplicatedMontageActionInstanceId = 0;
    bool bFocusActive = false;

    UPROPERTY(Transient)
    TMap<TObjectPtr<UAnimMontage>, TObjectPtr<UAnimMontage>> RuntimeMontageOverrides;
};
