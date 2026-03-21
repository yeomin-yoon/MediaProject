#pragma once

#include "Engine/EngineTypes.h"
#include "GameplayTagContainer.h"

#include "ActionCombatMeleeTraceTypes.generated.h"

UENUM(BlueprintType)
enum class EActionCombatMeleeHitDedupeMode : uint8
{
    None,
    OncePerWindow
};

USTRUCT(BlueprintType)
struct ACTIONCOMBATRUNTIME_API FActionCombatTracePoint
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace")
    FName SocketName = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace")
    FVector LocalOffset = FVector::ZeroVector;
};

USTRUCT(BlueprintType)
struct ACTIONCOMBATRUNTIME_API FActionCombatMeleeTraceProfile
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace")
    TArray<FActionCombatTracePoint> TracePoints;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace", meta = (ClampMin = "0.0"))
    float SweepRadius = 8.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace")
    TEnumAsByte<ECollisionChannel> TraceChannel = ECC_GameTraceChannel4;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace", meta = (ClampMin = "1"))
    int32 MaxHitResultsPerTick = 16;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace", meta = (ClampMin = "1"))
    int32 MaxUniqueTargetsPerWindow = 8;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace")
    bool bTraceComplex = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace")
    bool bIgnoreOwnerAttachedActors = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace")
    bool bStopAtBlockingHit = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace")
    EActionCombatMeleeHitDedupeMode DedupeMode = EActionCombatMeleeHitDedupeMode::OncePerWindow;

    bool IsValid() const
    {
        return TracePoints.Num() > 0 && SweepRadius >= 0.0f;
    }
};

USTRUCT(BlueprintType)
struct ACTIONCOMBATRUNTIME_API FActionCombatRecordedHit
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Hit")
    FHitResult HitResult;

    UPROPERTY(BlueprintReadOnly, Category = "Hit")
    FGameplayTag HitZoneTag;

    UPROPERTY(BlueprintReadOnly, Category = "Hit")
    float DamageMultiplier = 1.0f;
};
