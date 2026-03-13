#pragma once

#include "CoreMinimal.h"
#include "Components/PawnComponent.h"
#include "LockOnTypes.h"

#include "LockOnComponent.generated.h"

class ULockOnTargetComponent;
class ULockOnWorldSubsystem;

UCLASS(ClassGroup = (LockOn), BlueprintType, meta = (BlueprintSpawnableComponent))
class LOCKONSYSTEMRUNTIME_API ULockOnComponent : public UPawnComponent
{
    GENERATED_BODY()

public:
    ULockOnComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(BlueprintCallable, Category = "LockOn")
    void RequestToggleLock();

    UFUNCTION(BlueprintCallable, Category = "LockOn")
    void RequestCycleLeft();

    UFUNCTION(BlueprintCallable, Category = "LockOn")
    void RequestCycleRight();

    UFUNCTION(BlueprintCallable, Category = "LockOn")
    void RequestCycleUp();

    UFUNCTION(BlueprintCallable, Category = "LockOn")
    void RequestCycleDown();

    UFUNCTION(BlueprintCallable, Category = "LockOn")
    void RequestClearLock();

    UFUNCTION(BlueprintPure, Category = "LockOn")
    bool IsLockActive() const { return ReplicatedState.bIsLocked && (ReplicatedState.TargetActor != nullptr); }

    UFUNCTION(BlueprintPure, Category = "LockOn")
    AActor* GetCurrentTargetActor() const { return ReplicatedState.TargetActor; }

    UFUNCTION(BlueprintPure, Category = "LockOn")
    AActor* GetPreviewTargetActor() const;

    UFUNCTION(BlueprintCallable, Category = "LockOn")
    bool GetCurrentTargetFocusLocation(FVector& OutFocusLocation) const;

    UFUNCTION(BlueprintCallable, Category = "LockOn")
    void SetDebugDrawEnabled(bool bEnabled);

    UFUNCTION(BlueprintPure, Category = "LockOn")
    bool IsDebugDrawEnabled() const { return bDebugDrawEnabled; }

    UFUNCTION(BlueprintCallable, Category = "LockOn")
    void DumpDebugState() const;

    UFUNCTION(BlueprintCallable, Category = "LockOn")
    void RunBenchmark(int32 Iterations);

    void NotifyDelegateChainRecovered();

protected:
    UFUNCTION(Server, Reliable)
    void ServerRequestToggleLock();
    void ServerRequestToggleLock_Implementation();

    UFUNCTION(Server, Reliable)
    void ServerRequestCycle(ELockOnCycleDirection CycleDirection);
    void ServerRequestCycle_Implementation(ELockOnCycleDirection CycleDirection);

    UFUNCTION(Server, Reliable)
    void ServerRequestClearLock();
    void ServerRequestClearLock_Implementation();

private:
    UFUNCTION()
    void OnRep_ReplicatedState();

    bool AcquireBestTargetServer();
    bool CycleTargetServer(ELockOnCycleDirection CycleDirection);
    void ClearLockServer(ELockOnBreakReason BreakReason);
    void MaintainCurrentLockServer();

    bool GetViewPoint(FVector& OutViewLocation, FRotator& OutViewRotation) const;
    bool EvaluateCandidate(ULockOnTargetComponent* TargetComponent, const FVector& ViewLocation, const FRotator& ViewRotation, bool bIsMaintainPass, bool bAllowVisibilityTrace, bool bApplyStickyBonus, FLockOnCandidate& OutCandidate);
    bool QueryAndScoreCandidates(const FVector& ViewLocation, const FRotator& ViewRotation, bool bIsMaintainPass, bool bAllowVisibilityTrace, bool bApplyStickyBonus, bool bUseSpatialQuery, TArray<FLockOnCandidate>& OutCandidates, int32& OutTotalRegistered);
    bool ChooseBestCandidate(const TArray<FLockOnCandidate>& Candidates, FLockOnCandidate& OutBestCandidate) const;
    bool ChooseCycleCandidate(const TArray<FLockOnCandidate>& Candidates, ELockOnCycleDirection CycleDirection, FLockOnCandidate& OutBestCandidate) const;
    bool CanProcessServerCycleRequest(ELockOnCycleDirection CycleDirection);
    bool IsTargetStillValid(ULockOnTargetComponent* TargetComponent, const FVector& ViewLocation, const FRotator& ViewRotation, ELockOnBreakReason& OutBreakReason);
    bool PerformVisibilityTrace(const FVector& Start, const FVector& End, const AActor* TargetActor);
    void CommitLockState(AActor* NewTargetActor, ELockOnBreakReason BreakReason);
    void HandleLockStateChanged();
    void UpdateLocalPreview(float DeltaTime);
    void UpdateLocalHighlight();
    void ClearLocalHighlight();
    void SetHighlightedActor(AActor* NewHighlightedActor);
    void DrawDebugState();
    void RefreshDebugCandidatesFromLastQuery(const TArray<FLockOnCandidate>& Candidates);

    bool ShouldRunLocalPreview() const;
    bool ShouldRunServerAuthority() const;
    ULockOnWorldSubsystem* GetLockOnWorldSubsystem() const;
    ULockOnTargetComponent* GetCurrentTargetComponent() const;
    void RecordPreviewMismatchIfNeeded(AActor* AuthoritativeTargetActor);

private:
    UPROPERTY(ReplicatedUsing = OnRep_ReplicatedState)
    FLockOnRepState ReplicatedState;

    bool bDebugDrawEnabled = false;
    float PreviewRefreshAccumulator = 0.0f;
    float MaintainCheckAccumulator = 0.0f;
    float MaintainVisibilityAccumulator = 0.0f;
    float SecondsSinceVisibilityLoss = -1.0f;
    float LastServerCycleRequestWorldTime = -1.0f;
    int32 DelegateChainRecoveryCount = 0;

    TWeakObjectPtr<AActor> LocalPreviewTargetActor;
    TWeakObjectPtr<AActor> HighlightedActor;
    TArray<FLockOnPrimitiveHighlightState> HighlightedPrimitiveStates;
    TArray<FLockOnCandidate> LastScoredCandidates;

    int32 QueryCount = 0;
    int32 VisibilityTraceCount = 0;
    int32 MaintainCheckCount = 0;
    int32 PreviewMismatchCount = 0;
    int32 RejectedCycleRequestCount = 0;
    int32 BreakReasonCounts[(int32)ELockOnBreakReason::MAX] = { 0 };
};
