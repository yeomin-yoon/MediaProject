#pragma once

#include "AttributeSet.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "GameplayAbilitySpec.h"

#include "ActionCombatLyraGuardComponent.generated.h"

class AActor;
class UAbilitySystemComponent;
class UActionCombatComponent;

UENUM(BlueprintType)
enum class EActionCombatLyraGuardOutcome : uint8
{
    None,
    Blocked,
    GuardBroken
};

USTRUCT(BlueprintType)
struct ACTIONCOMBATLYRABRIDGE_API FActionCombatLyraIncomingGuardHit
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard")
    TObjectPtr<AActor> Attacker = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard")
    FHitResult HitResult;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard")
    bool bBlockable = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard", meta = (ClampMin = "0.0"))
    float GuardDamage = 20.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard", meta = (ClampMin = "0.0"))
    float ForcedGuardDurationSeconds = 0.35f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard", meta = (ClampMin = "0.0"))
    float GuardBreakDurationSeconds = 0.9f;
};

USTRUCT(BlueprintType)
struct ACTIONCOMBATLYRABRIDGE_API FActionCombatLyraGuardResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Guard")
    EActionCombatLyraGuardOutcome Outcome = EActionCombatLyraGuardOutcome::None;

    UPROPERTY(BlueprintReadOnly, Category = "Guard")
    float AppliedGuardDamage = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Guard")
    float ResourceBefore = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Guard")
    float ResourceAfter = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Guard")
    float FacingDot = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Guard")
    bool bDamagePrevented = false;
};

USTRUCT(BlueprintType)
struct ACTIONCOMBATLYRABRIDGE_API FActionCombatLyraReplicatedGuardState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Guard")
    bool bGuardInputHeld = false;

    UPROPERTY(BlueprintReadOnly, Category = "Guard")
    bool bGuardActive = false;

    UPROPERTY(BlueprintReadOnly, Category = "Guard")
    bool bForcedGuardActive = false;

    UPROPERTY(BlueprintReadOnly, Category = "Guard")
    bool bGuardBroken = false;

    bool Equals(const FActionCombatLyraReplicatedGuardState& Other) const
    {
        return bGuardInputHeld == Other.bGuardInputHeld
            && bGuardActive == Other.bGuardActive
            && bForcedGuardActive == Other.bForcedGuardActive
            && bGuardBroken == Other.bGuardBroken;
    }
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FActionCombatLyraGuardStateChangedSignature, FActionCombatLyraReplicatedGuardState, GuardState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FActionCombatLyraGuardBlockedSignature, EActionCombatLyraGuardOutcome, Outcome, float, AppliedGuardDamage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FActionCombatLyraGuardBrokenSignature);

UCLASS(BlueprintType, ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class ACTIONCOMBATLYRABRIDGE_API UActionCombatLyraGuardComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UActionCombatLyraGuardComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(BlueprintCallable, Category = "Action Combat|Guard")
    void SetGuardInputHeld(bool bNewGuardInputHeld);

    UFUNCTION(BlueprintPure, Category = "Action Combat|Guard")
    bool IsGuardInputHeld() const
    {
        return GuardState.bGuardInputHeld;
    }

    UFUNCTION(BlueprintPure, Category = "Action Combat|Guard")
    bool IsGuardActive() const
    {
        return GuardState.bGuardActive;
    }

    UFUNCTION(BlueprintPure, Category = "Action Combat|Guard")
    bool IsForcedGuardActive() const
    {
        return GuardState.bForcedGuardActive;
    }

    UFUNCTION(BlueprintPure, Category = "Action Combat|Guard")
    bool IsGuardBroken() const
    {
        return GuardState.bGuardBroken;
    }

    UFUNCTION(BlueprintPure, Category = "Action Combat|Guard")
    FActionCombatLyraReplicatedGuardState GetGuardState() const
    {
        return GuardState;
    }

    UFUNCTION(BlueprintCallable, Category = "Action Combat|Guard")
    bool TryResolveIncomingHit(const FActionCombatLyraIncomingGuardHit& IncomingHit, FActionCombatLyraGuardResult& OutResult);

    UFUNCTION(BlueprintCallable, Category = "Action Combat|Guard")
    void ForceGuardBreak(float RecoverySeconds = -1.0f);

    UFUNCTION(BlueprintCallable, Category = "Action Combat|Guard")
    void RefreshGuardStateTags();

    UFUNCTION(BlueprintPure, Category = "Action Combat|Guard")
    static UActionCombatLyraGuardComponent* FindGuardComponent(const AActor* Actor);

    UPROPERTY(BlueprintAssignable, Category = "Action Combat|Guard")
    FActionCombatLyraGuardStateChangedSignature OnGuardStateChanged;

    UPROPERTY(BlueprintAssignable, Category = "Action Combat|Guard")
    FActionCombatLyraGuardBlockedSignature OnGuardBlocked;

    UPROPERTY(BlueprintAssignable, Category = "Action Combat|Guard")
    FActionCombatLyraGuardBrokenSignature OnGuardBroken;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Guard")
    FComponentReference ActionCombatComponentReference;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Guard")
    FGameplayAttribute GuardResourceAttribute;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Guard", meta = (ClampMin = "0.0"))
    float GuardActivationDelaySeconds = 0.08f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Guard")
    bool bRequireFacingForBlock = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Guard", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
    float MinimumBlockDotProduct = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Guard", meta = (ClampMin = "0.0"))
    float DefaultForcedGuardDurationSeconds = 0.35f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Guard", meta = (ClampMin = "0.0"))
    float DefaultGuardBreakDurationSeconds = 0.9f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Guard")
    bool bDispatchGameplayEvents = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Guard")
    bool bInterruptCombatActionsOnGuardStart = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Guard")
    bool bInterruptCombatActionsOnGuardBreak = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Guard")
    bool bBlockGuardStartDuringCombatAction = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Guard")
    bool bLogGuardFlow = true;

private:
    UFUNCTION(Server, Reliable)
    void ServerSetGuardInputHeld(bool bNewGuardInputHeld);

    UFUNCTION()
    void OnRep_GuardState(FActionCombatLyraReplicatedGuardState PreviousState);

    void SetGuardInputHeldAuthority(bool bNewGuardInputHeld);
    void SetGuardActiveAuthority(bool bNewGuardActive);
    void StartOrRefreshForcedGuardAuthority(float DurationSeconds);
    void BreakGuardAuthority(float RecoverySeconds);
    void ClearAllTimers();
    void HandleGuardActivationTimerExpired();
    void HandleForcedGuardTimerExpired();
    void HandleGuardBrokenTimerExpired();
    void HandleGuardStateMutated(const FActionCombatLyraReplicatedGuardState& PreviousState);
    void BroadcastGuardStateChange(const FActionCombatLyraReplicatedGuardState& PreviousState);
    void UpdateGuardGameplayTags();
    void DispatchGuardGameplayEvent(const FGameplayTag& EventTag, const FActionCombatLyraIncomingGuardHit& IncomingHit, const FActionCombatLyraGuardResult& GuardResult);
    void InterruptCombatActionIfNeeded();
    bool IsCombatActionActive() const;
    bool HasGuardAuthority() const;
    float ResolveGuardDuration(float RequestedDurationSeconds, float DefaultDurationSeconds) const;
    float ComputeFacingDot(const FActionCombatLyraIncomingGuardHit& IncomingHit) const;
    UAbilitySystemComponent* ResolveAbilitySystemComponent() const;
    UActionCombatComponent* ResolveActionCombatComponent() const;
    void LogGuard(const FString& Message) const;

    UPROPERTY(ReplicatedUsing = OnRep_GuardState)
    FActionCombatLyraReplicatedGuardState GuardState;

    FTimerHandle GuardActivationTimerHandle;
    FTimerHandle ForcedGuardTimerHandle;
    FTimerHandle GuardBrokenTimerHandle;
};
