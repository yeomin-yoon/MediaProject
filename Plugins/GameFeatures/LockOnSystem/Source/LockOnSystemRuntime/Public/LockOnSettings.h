#pragma once

#include "Engine/DeveloperSettings.h"
#include "LockOnTypes.h"

#include "LockOnSettings.generated.h"

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Lock On"))
class LOCKONSYSTEMRUNTIME_API ULockOnSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    static const ULockOnSettings* Get();

    const FLockOnCameraProfile& GetCameraProfile(bool bUseLargeBossProfile) const;

    UPROPERTY(Config, EditAnywhere, Category = "LockOn|Spatial")
    float GridCellSize = 1200.0f;

    UPROPERTY(Config, EditAnywhere, Category = "LockOn|Spatial")
    int32 MaxCandidates = 32;

    UPROPERTY(Config, EditAnywhere, Category = "LockOn|Acquire")
    float AcquireRadius = 3000.0f;

    UPROPERTY(Config, EditAnywhere, Category = "LockOn|Maintain")
    float MaintainRadius = 3600.0f;

    UPROPERTY(Config, EditAnywhere, Category = "LockOn|Acquire")
    float AcquireHalfAngleDegrees = 50.0f;

    UPROPERTY(Config, EditAnywhere, Category = "LockOn|Maintain")
    float MaintainHalfAngleDegrees = 75.0f;

    UPROPERTY(Config, EditAnywhere, Category = "LockOn|Acquire")
    float MaxHeightDelta = 1200.0f;

    UPROPERTY(Config, EditAnywhere, Category = "LockOn|Maintain")
    float BreakGraceSeconds = 0.25f;

    UPROPERTY(Config, EditAnywhere, Category = "LockOn|Maintain")
    float MaintainCheckIntervalSeconds = 0.05f;

    UPROPERTY(Config, EditAnywhere, Category = "LockOn|Maintain")
    float MaintainVisibilityCheckIntervalSeconds = 0.20f;

    UPROPERTY(Config, EditAnywhere, Category = "LockOn|Preview")
    float PreviewRefreshIntervalSeconds = 0.10f;

    UPROPERTY(Config, EditAnywhere, Category = "LockOn|Visibility")
    ELockOnVisibilityTracePolicy VisibilityTracePolicy = ELockOnVisibilityTracePolicy::AcquireAndCycleOnly;

    UPROPERTY(Config, EditAnywhere, Category = "LockOn|Debug")
    bool bEnableDebugDrawByDefault = false;

    UPROPERTY(Config, EditAnywhere, Category = "LockOn|Debug")
    int32 MaxDebugCandidatesToDraw = 5;

    UPROPERTY(Config, EditAnywhere, Category = "LockOn|Preview")
    bool bEnablePreviewCustomDepth = true;

    UPROPERTY(Config, EditAnywhere, Category = "LockOn|Preview")
    int32 PreviewCustomDepthStencilValue = 252;

    UPROPERTY(Config, EditAnywhere, Category = "LockOn|Input")
    bool bBindMiddleMouseToggle = true;

    UPROPERTY(Config, EditAnywhere, Category = "LockOn|Input")
    bool bEnableMouseMoveCycle = true;

    UPROPERTY(Config, EditAnywhere, Category = "LockOn|Input", meta = (ClampMin = "0.01"))
    float MouseCycleThreshold = 12.0f;

    UPROPERTY(Config, EditAnywhere, Category = "LockOn|Input", meta = (ClampMin = "0.0"))
    float MouseCycleDeadZone = 0.01f;

    UPROPERTY(Config, EditAnywhere, Category = "LockOn|Input", meta = (ClampMin = "0.0"))
    float MouseCycleCooldownSeconds = 0.08f;

    UPROPERTY(Config, EditAnywhere, Category = "LockOn|Input", meta = (ClampMin = "0.0"))
    float MouseCycleIdleResetSeconds = 0.20f;

    UPROPERTY(Config, EditAnywhere, Category = "LockOn|Authority", meta = (ClampMin = "0.0"))
    float ServerCycleRequestCooldownSeconds = 0.05f;

    UPROPERTY(Config, EditAnywhere, Category = "LockOn|Scoring")
    FLockOnScoreWeights ScoreWeights;

    UPROPERTY(Config, EditAnywhere, Category = "LockOn|Camera")
    FLockOnCameraProfile DuelCameraProfile;

    UPROPERTY(Config, EditAnywhere, Category = "LockOn|Camera")
    FLockOnCameraProfile LargeBossCameraProfile;
};
