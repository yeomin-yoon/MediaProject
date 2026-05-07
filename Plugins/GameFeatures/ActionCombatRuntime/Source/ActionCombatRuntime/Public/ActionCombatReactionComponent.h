#pragma once

#include "Components/ActorComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/SoftObjectPtr.h"

#include "ActionCombatReactionComponent.generated.h"

class AActor;
class UAnimInstance;
class UAbilitySystemComponent;
class UAnimMontage;
class UAnimSequenceBase;
class UActionCombatReactionSet;
class USkeletalMeshComponent;

UENUM(BlueprintType)
enum class EActionCombatReactionOutcome : uint8
{
    None,
    LightHit,
    HeavyHit,
    Knockdown
};

UENUM(BlueprintType)
enum class EActionCombatReactionState : uint8
{
    None,
    LightHit,
    HeavyHit,
    Knockdown,
    GetUp
};

USTRUCT(BlueprintType)
struct ACTIONCOMBATRUNTIME_API FActionCombatReactionHit
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "Action Combat|Reaction")
    float PoiseDamage = 0.0f;

    UPROPERTY(BlueprintReadWrite, Category = "Action Combat|Reaction")
    float KnockdownPower = 0.0f;

    UPROPERTY(BlueprintReadWrite, Category = "Action Combat|Reaction")
    FVector WorldSpaceImpulseDirection = FVector::ZeroVector;

    UPROPERTY(BlueprintReadWrite, Category = "Action Combat|Reaction")
    TObjectPtr<AActor> InstigatorActor = nullptr;

    bool HasMeaningfulReaction() const
    {
        return (PoiseDamage > KINDA_SMALL_NUMBER) || (KnockdownPower > KINDA_SMALL_NUMBER);
    }
};

UENUM(BlueprintType)
enum class EActionCombatReactionDirection : uint8
{
    Front,
    Back,
    Left,
    Right
};

USTRUCT(BlueprintType)
struct ACTIONCOMBATRUNTIME_API FActionCombatReactionAnimation
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Reaction")
    TSoftObjectPtr<UAnimMontage> Montage;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Reaction")
    TSoftObjectPtr<UAnimSequenceBase> Sequence;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Reaction", meta = (ClampMin = "0.01"))
    float PlayRate = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Reaction", meta = (ClampMin = "0.0"))
    float BlendInSeconds = 0.05f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Reaction", meta = (ClampMin = "0.0"))
    float BlendOutSeconds = 0.08f;

    bool HasAnimation() const
    {
        return !Montage.IsNull() || !Sequence.IsNull();
    }
};

USTRUCT(BlueprintType)
struct ACTIONCOMBATRUNTIME_API FActionCombatDirectionalReactionAnimations
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Reaction")
    FActionCombatReactionAnimation Front;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Reaction")
    FActionCombatReactionAnimation Back;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Reaction")
    FActionCombatReactionAnimation Left;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Reaction")
    FActionCombatReactionAnimation Right;

    const FActionCombatReactionAnimation* FindAnimation(EActionCombatReactionDirection Direction) const;
};

USTRUCT(BlueprintType)
struct ACTIONCOMBATRUNTIME_API FActionCombatReactionResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Action Combat|Reaction")
    EActionCombatReactionOutcome Outcome = EActionCombatReactionOutcome::None;

    UPROPERTY(BlueprintReadOnly, Category = "Action Combat|Reaction")
    float PoiseBefore = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Action Combat|Reaction")
    float PoiseAfter = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Action Combat|Reaction")
    int32 PoiseBreakChainCount = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Action Combat|Reaction")
    bool bInterruptedCombatAction = false;
};

UCLASS(BlueprintType, ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class ACTIONCOMBATRUNTIME_API UActionCombatReactionComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UActionCombatReactionComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(BlueprintPure, Category = "Action Combat|Reaction")
    static UActionCombatReactionComponent* FindReactionComponent(const AActor* Actor);

    static UActionCombatReactionComponent* FindOrCreateReactionComponent(AActor* Actor);
    static void PlayReactionCueOnActor(AActor* Actor, EActionCombatReactionState NewState, FVector_NetQuantizeNormal WorldSpaceImpulseDirection, FVector_NetQuantize WorldSpaceActorLocation, int32 CueId);

    UFUNCTION(BlueprintPure, Category = "Action Combat|Reaction")
    bool HasReactionAuthority() const;

    UFUNCTION(BlueprintPure, Category = "Action Combat|Reaction")
    EActionCombatReactionOutcome GetActiveReactionOutcome() const;

    UFUNCTION(BlueprintPure, Category = "Action Combat|Reaction")
    bool IsInReactionState() const;

    bool TryApplyReactionHit(const FActionCombatReactionHit& IncomingHit, FActionCombatReactionResult& OutResult);
    void PlayReplicatedReactionCue(EActionCombatReactionState NewState, FVector_NetQuantizeNormal WorldSpaceImpulseDirection, FVector_NetQuantize WorldSpaceActorLocation, int32 CueId);

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Reaction", meta = (ClampMin = "0.0"))
    float LightHitDurationSeconds = 0.20f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Reaction", meta = (ClampMin = "0.0"))
    float HeavyHitDurationSeconds = 0.45f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Reaction", meta = (ClampMin = "0.0"))
    float KnockdownDurationSeconds = 0.95f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Reaction", meta = (ClampMin = "0.0"))
    float GetUpDurationSeconds = 1.55f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Reaction", meta = (ClampMin = "0.0"))
    float PoiseRecoveryDelaySeconds = 0.75f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Reaction", meta = (ClampMin = "0.0"))
    float PostReactionImmunitySeconds = 0.10f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Reaction", meta = (ClampMin = "0.0"))
    float PostGetUpReactionImmunitySeconds = 0.35f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Reaction", meta = (ClampMin = "0.0"))
    float PoiseBreakChainResetSeconds = 1.50f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Reaction", meta = (ClampMin = "1"))
    int32 PoiseBreaksBeforeKnockdown = 2;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Reaction|Fallback", meta = (ClampMin = "1.0"))
    float FallbackMaxPoise = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Reaction|Fallback", meta = (ClampMin = "0.0"))
    float FallbackPoiseRecoveryRate = 20.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Reaction|Fallback", meta = (ClampMin = "0.0"))
    float FallbackKnockdownThreshold = 60.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Reaction", meta = (ClampMin = "0.0"))
    float KnockdownLaunchSpeed = 350.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Reaction", meta = (ClampMin = "0.0"))
    float KnockdownUpwardLaunchSpeed = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Reaction", meta = (ClampMin = "0.0"))
    float KnockdownActorDisplacementDistance = 280.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Reaction", meta = (ClampMin = "0.01"))
    float KnockdownActorDisplacementDurationSeconds = 0.85f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Reaction")
    bool bFaceReactionSourceOnKnockdown = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Reaction")
    bool bInterruptCombatActionsOnReaction = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Reaction")
    bool bDisableMovementDuringHitReaction = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Reaction|Animation")
    bool bAutoPlayReactionAnimations = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Reaction|Animation")
    FName ReactionSlotName = TEXT("FullBody");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Reaction|Animation")
    bool bStopPreviousReactionAnimation = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Reaction|Animation")
    bool bHoldKnockdownPoseUntilGetUp = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Reaction|Animation", meta = (ClampMin = "0.0"))
    float KnockdownPoseHoldLeadSeconds = 0.02f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Reaction|Animation", meta = (ClampMin = "0.0"))
    float GetUpHoldReleaseDelaySeconds = 0.08f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Reaction|Animation")
    bool bSnapClientToServerReactionLocation = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Reaction|Animation")
    FActionCombatDirectionalReactionAnimations LightHitAnimations;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Reaction|Animation")
    FActionCombatDirectionalReactionAnimations HeavyHitAnimations;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Reaction|Animation")
    FActionCombatReactionAnimation KnockdownAnimation;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Reaction|Animation")
    FActionCombatReactionAnimation GetUpAnimation;

private:
    UAbilitySystemComponent* ResolveAbilitySystemComponent() const;
    UActionCombatReactionSet* FindReactionSet() const;
    UActionCombatReactionSet* EnsureReactionSet();
    float GetCurrentPoise() const;
    float GetMaxPoise() const;
    float GetPoiseRecoveryRate() const;
    float GetKnockdownThreshold() const;
    void SetCurrentPoise(float NewValue) const;
    void ClearReactionTags() const;
    void UpdateReactionTags() const;
    void BeginTimedReaction(EActionCombatReactionState NewState, float DurationSeconds, const FVector& WorldSpaceImpulseDirection);
    void BeginKnockdown(const FVector& WorldSpaceImpulseDirection);
    void BeginGetUp();
    void FinishReaction(float AdditionalImmunitySeconds);
    void HandleReactionTimerExpired();
    bool InterruptCombatAction() const;
    void ApplyMovementLock(bool bLockMovement);
    void ApplyReactionFacing(const FVector& WorldSpaceImpulseDirection);
    void ApplyKnockdownLaunch(const FVector& WorldSpaceImpulseDirection);
    void StartKnockdownActorDisplacement(const FVector& WorldSpaceImpulseDirection, float DurationSeconds);
    void TickKnockdownActorDisplacement(float DeltaTime);
    void ClearKnockdownActorDisplacement();
    void StartReactionCue(EActionCombatReactionState NewState, const FVector& WorldSpaceImpulseDirection);
    void PlayReactionAnimation(EActionCombatReactionState ReactionState, const FVector& WorldSpaceImpulseDirection);
    void ClearKnockdownPoseHold(bool bStopHeldMontage);
    void ClearKnockdownPoseHoldDeferred();
    void ScheduleKnockdownPoseHold(UAnimInstance* AnimInstance, UAnimMontage* KnockdownMontage, float PlayRate);
    void HoldKnockdownPose();
    void ApplyReplicatedReactionLocation(EActionCombatReactionState NewState, const FVector& WorldSpaceActorLocation);
    const FActionCombatReactionAnimation* FindReactionAnimation(EActionCombatReactionState ReactionState, EActionCombatReactionDirection Direction) const;
    EActionCombatReactionDirection ResolveReactionDirection(const FVector& WorldSpaceImpulseDirection) const;
    USkeletalMeshComponent* ResolveAnimationMesh(const FActionCombatReactionAnimation* ReactionAnimation) const;
    bool IsAnimationCompatibleWithMesh(const USkeletalMeshComponent* MeshComponent, const FActionCombatReactionAnimation* ReactionAnimation) const;
    float GetReactionAnimationPlayLengthSeconds(EActionCombatReactionState ReactionState, const FVector& WorldSpaceImpulseDirection, float FallbackSeconds) const;
    double GetCurrentWorldTimeSeconds() const;

    UFUNCTION()
    void OnRep_ReplicatedReactionCue();

    UFUNCTION(NetMulticast, Reliable)
    void MulticastPlayReactionCue(EActionCombatReactionState NewState, FVector_NetQuantizeNormal WorldSpaceImpulseDirection, FVector_NetQuantize WorldSpaceActorLocation, int32 CueId);

    mutable TWeakObjectPtr<UActionCombatReactionSet> CachedReactionSet;
    FTimerHandle ReactionTimerHandle;
    FTimerHandle KnockdownPoseHoldTimerHandle;
    FTimerHandle KnockdownPoseHoldClearTimerHandle;
    UPROPERTY(Transient)
    TObjectPtr<UAnimMontage> ActiveReactionMontage;
    UPROPERTY(Transient)
    TObjectPtr<UAnimMontage> HeldKnockdownMontage;
    TWeakObjectPtr<UAnimInstance> ActiveReactionAnimInstance;
    UPROPERTY(Replicated)
    EActionCombatReactionState ReplicatedReactionCueState = EActionCombatReactionState::None;
    UPROPERTY(Replicated)
    FVector_NetQuantizeNormal ReplicatedReactionCueDirection = FVector::ZeroVector;
    UPROPERTY(Replicated)
    FVector_NetQuantize ReplicatedReactionCueLocation = FVector::ZeroVector;
    UPROPERTY(ReplicatedUsing = OnRep_ReplicatedReactionCue)
    int32 ReplicatedReactionCueId = 0;
    TEnumAsByte<EMovementMode> SavedMovementMode = MOVE_Walking;
    uint8 SavedCustomMovementMode = 0;
    FVector LastReactionImpulseDirection = FVector::ZeroVector;
    FVector KnockdownActorDisplacementDirection = FVector::ZeroVector;
    float KnockdownActorDisplacementRemainingDistance = 0.0f;
    float KnockdownActorDisplacementSpeed = 0.0f;
    TWeakObjectPtr<AActor> LastReactionInstigatorActor;
    int32 LastPlayedReactionCueId = 0;
    double LastIncomingHitWorldTimeSeconds = -1.0;
    double LastPoiseBreakWorldTimeSeconds = -1.0;
    double ReactionImmunityEndWorldTimeSeconds = -1.0;
    mutable float LocalPoise = -1.0f;
    int32 RecentPoiseBreakCount = 0;
    EActionCombatReactionState ActiveReactionState = EActionCombatReactionState::None;
    bool bMovementLocked = false;
};
