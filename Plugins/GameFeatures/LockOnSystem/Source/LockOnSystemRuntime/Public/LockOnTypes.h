#pragma once

#include "CoreMinimal.h"
#include "LockOnTypes.generated.h"

class AActor;
class ULockOnTargetComponent;

UENUM(BlueprintType)
enum class ELockOnCycleDirection : uint8
{
    Left,
    Right,
    Up,
    Down
};

UENUM(BlueprintType)
enum class ELockOnBreakReason : uint8
{
    None,
    ExplicitClear,
    InvalidTarget,
    OutOfRange,
    OutOfCone,
    Occluded,
    TargetDestroyed,
    MAX UMETA(Hidden)
};

UENUM(BlueprintType)
enum class ELockOnVisibilityTracePolicy : uint8
{
    Disabled,
    AcquireAndCycleOnly,
    AcquireCycleAndLowRateMaintain
};

USTRUCT(BlueprintType)
struct FLockOnScoreWeights
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn")
    float ScreenCenterWeight = 0.55f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn")
    float DistanceWeight = 0.25f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn")
    float VisibilityWeight = 0.10f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn")
    float StickyWeight = 0.20f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn")
    float TargetPriorityWeight = 1.00f;
};

USTRUCT(BlueprintType)
struct FLockOnCameraProfile
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn")
    FVector AdditionalTargetOffset = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn")
    float YawInterpSpeedDegPerSecond = 540.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn")
    float PitchInterpSpeedDegPerSecond = 360.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn")
    float PitchBiasDegrees = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn")
    float FieldOfViewOffset = 0.0f;
};

USTRUCT(BlueprintType)
struct FLockOnCandidateScoreBreakdown
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LockOn")
    float ScreenCenterScore = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LockOn")
    float DistanceScore = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LockOn")
    float VisibilityScore = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LockOn")
    float StickyBonus = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LockOn")
    float TargetPriorityBonus = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LockOn")
    float TotalScore = 0.0f;
};

USTRUCT(BlueprintType)
struct FLockOnRepState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "LockOn")
    TObjectPtr<AActor> TargetActor = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "LockOn")
    int32 Sequence = 0;

    UPROPERTY(BlueprintReadOnly, Category = "LockOn")
    bool bIsLocked = false;

    UPROPERTY(BlueprintReadOnly, Category = "LockOn")
    ELockOnBreakReason BreakReason = ELockOnBreakReason::None;
};

struct FLockOnCandidate
{
    TWeakObjectPtr<ULockOnTargetComponent> TargetComponent;
    FVector FocusLocation = FVector::ZeroVector;
    float Distance = 0.0f;
    float ViewDot = 0.0f;
    bool bVisibilityConfirmed = false;
    FLockOnCandidateScoreBreakdown ScoreBreakdown;
};

struct FLockOnPrimitiveHighlightState
{
    TWeakObjectPtr<class UPrimitiveComponent> PrimitiveComponent;
    bool bOriginalRenderCustomDepth = false;
    int32 OriginalStencilValue = 0;
};
