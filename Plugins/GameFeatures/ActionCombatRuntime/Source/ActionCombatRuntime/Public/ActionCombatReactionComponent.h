#pragma once

#include "Components/ActorComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "ActionCombatReactionComponent.generated.h"

class AActor;
class UAbilitySystemComponent;
class UActionCombatReactionSet;

UENUM(BlueprintType)
enum class EActionCombatReactionOutcome : uint8
{
    None,
    LightHit,
    HeavyHit,
    Knockdown
};

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

    UFUNCTION(BlueprintPure, Category = "Action Combat|Reaction")
    static UActionCombatReactionComponent* FindReactionComponent(const AActor* Actor);

    static UActionCombatReactionComponent* FindOrCreateReactionComponent(AActor* Actor);

    UFUNCTION(BlueprintPure, Category = "Action Combat|Reaction")
    bool HasReactionAuthority() const;

    UFUNCTION(BlueprintPure, Category = "Action Combat|Reaction")
    EActionCombatReactionOutcome GetActiveReactionOutcome() const;

    UFUNCTION(BlueprintPure, Category = "Action Combat|Reaction")
    bool IsInReactionState() const;

    bool TryApplyReactionHit(const FActionCombatReactionHit& IncomingHit, FActionCombatReactionResult& OutResult);

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Reaction", meta = (ClampMin = "0.0"))
    float LightHitDurationSeconds = 0.20f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Reaction", meta = (ClampMin = "0.0"))
    float HeavyHitDurationSeconds = 0.45f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Reaction", meta = (ClampMin = "0.0"))
    float KnockdownDurationSeconds = 0.90f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Reaction", meta = (ClampMin = "0.0"))
    float GetUpDurationSeconds = 0.55f;

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

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Reaction", meta = (ClampMin = "0.0"))
    float KnockdownLaunchSpeed = 500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Reaction", meta = (ClampMin = "0.0"))
    float KnockdownUpwardLaunchSpeed = 160.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Reaction")
    bool bInterruptCombatActionsOnReaction = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Reaction")
    bool bDisableMovementDuringHitReaction = true;

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
    void BeginTimedReaction(EActionCombatReactionState NewState, float DurationSeconds);
    void BeginKnockdown(const FVector& WorldSpaceImpulseDirection);
    void BeginGetUp();
    void FinishReaction(float AdditionalImmunitySeconds);
    void HandleReactionTimerExpired();
    bool InterruptCombatAction() const;
    void ApplyMovementLock(bool bLockMovement);
    void ApplyKnockdownLaunch(const FVector& WorldSpaceImpulseDirection);
    double GetCurrentWorldTimeSeconds() const;

    mutable TWeakObjectPtr<UActionCombatReactionSet> CachedReactionSet;
    FTimerHandle ReactionTimerHandle;
    TEnumAsByte<EMovementMode> SavedMovementMode = MOVE_Walking;
    uint8 SavedCustomMovementMode = 0;
    double LastIncomingHitWorldTimeSeconds = -1.0;
    double LastPoiseBreakWorldTimeSeconds = -1.0;
    double ReactionImmunityEndWorldTimeSeconds = -1.0;
    int32 RecentPoiseBreakCount = 0;
    EActionCombatReactionState ActiveReactionState = EActionCombatReactionState::None;
    bool bMovementLocked = false;
};
