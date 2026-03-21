#include "ActionCombatMeleeTraceComponent.h"

#include "ActionCombatHurtboxComponent.h"
#include "ActionCombatRuntimeLog.h"

#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Actor.h"
#include "UObject/ObjectKey.h"

UActionCombatMeleeTraceComponent::UActionCombatMeleeTraceComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UActionCombatMeleeTraceComponent::BeginPlay()
{
    Super::BeginPlay();

    SetComponentTickEnabled(false);
}

void UActionCombatMeleeTraceComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!bHitWindowActive)
    {
        SetComponentTickEnabled(false);
        return;
    }

    if (!ShouldRunTraceAuthority())
    {
        return;
    }

    TraceActiveWindow();
}

bool UActionCombatMeleeTraceComponent::StartHitWindowWithDefaultProfile(FName WindowName)
{
    return StartHitWindowInternal(WindowName, DefaultTraceProfile);
}

bool UActionCombatMeleeTraceComponent::StartHitWindowWithProfile(FName WindowName, const FActionCombatMeleeTraceProfile& TraceProfile)
{
    return StartHitWindowInternal(WindowName, TraceProfile);
}

void UActionCombatMeleeTraceComponent::StopHitWindow()
{
    if (!bHitWindowActive)
    {
        return;
    }

    const FName ClosedWindowName = ActiveWindowName;

    bHitWindowActive = false;
    ActiveWindowName = NAME_None;
    ActiveTraceProfile = FActionCombatMeleeTraceProfile();
    PreviousTraceLocations.Reset();
    HitActorsThisWindow.Reset();
    SetComponentTickEnabled(false);

    OnHitWindowStopped.Broadcast(this, ClosedWindowName);
}

void UActionCombatMeleeTraceComponent::SetDefaultTraceProfile(const FActionCombatMeleeTraceProfile& NewProfile)
{
    DefaultTraceProfile = NewProfile;
}

USceneComponent* UActionCombatMeleeTraceComponent::ResolveTraceSourceComponent() const
{
    if (CachedTraceSourceComponent.IsValid())
    {
        return CachedTraceSourceComponent.Get();
    }

    if (AActor* Owner = GetOwner())
    {
        if (USceneComponent* ExplicitComponent = Cast<USceneComponent>(TraceSourceComponent.GetComponent(Owner)))
        {
            return ExplicitComponent;
        }

        TArray<USkeletalMeshComponent*> SkeletalMeshes;
        Owner->GetComponents(SkeletalMeshes);
        if (SkeletalMeshes.Num() > 0)
        {
            return SkeletalMeshes[0];
        }

        return Owner->GetRootComponent();
    }

    return nullptr;
}

void UActionCombatMeleeTraceComponent::ClearRecordedHits()
{
    RecordedHits.Reset();
}

void UActionCombatMeleeTraceComponent::ConsumeRecordedHits(TArray<FActionCombatRecordedHit>& OutHits)
{
    OutHits = RecordedHits;
    RecordedHits.Reset();
}

void UActionCombatMeleeTraceComponent::ConsumeRecordedHitResults(TArray<FHitResult>& OutHitResults)
{
    OutHitResults.Reset();
    OutHitResults.Reserve(RecordedHits.Num());

    for (const FActionCombatRecordedHit& RecordedHit : RecordedHits)
    {
        OutHitResults.Add(RecordedHit.HitResult);
    }

    RecordedHits.Reset();
}

FGameplayAbilityTargetDataHandle UActionCombatMeleeTraceComponent::ConsumeRecordedTargetData()
{
    FGameplayAbilityTargetDataHandle TargetData;

    for (const FActionCombatRecordedHit& RecordedHit : RecordedHits)
    {
        FGameplayAbilityTargetData_SingleTargetHit* NewTargetData = new FGameplayAbilityTargetData_SingleTargetHit();
        NewTargetData->HitResult = RecordedHit.HitResult;
        TargetData.Add(NewTargetData);
    }

    RecordedHits.Reset();
    return TargetData;
}

bool UActionCombatMeleeTraceComponent::StartHitWindowInternal(FName WindowName, const FActionCombatMeleeTraceProfile& TraceProfile)
{
    if (!ShouldRunTraceAuthority())
    {
        return false;
    }

    if (!TraceProfile.IsValid())
    {
        UE_LOG(LogActionCombatRuntime, Warning, TEXT("%s cannot start melee hit window '%s' because the trace profile is invalid."), *GetPathName(), *WindowName.ToString());
        return false;
    }

    if (bHitWindowActive)
    {
        StopHitWindow();
    }

    CachedTraceSourceComponent = ResolveTraceSourceComponent();
    if (!CachedTraceSourceComponent.IsValid())
    {
        UE_LOG(LogActionCombatRuntime, Warning, TEXT("%s cannot start melee hit window '%s' because no trace source component could be resolved."), *GetPathName(), *WindowName.ToString());
        return false;
    }

    ActiveTraceProfile = TraceProfile;
    ActiveWindowName = WindowName;
    bHitWindowActive = true;
    PreviousTraceLocations.Reset();
    PreviousTraceLocations.SetNum(ActiveTraceProfile.TracePoints.Num());
    HitActorsThisWindow.Reset();
    RecordedHits.Reset();

    for (int32 Index = 0; Index < ActiveTraceProfile.TracePoints.Num(); ++Index)
    {
        FVector CurrentLocation = FVector::ZeroVector;
        if (!GetTracePointWorldLocation(ActiveTraceProfile.TracePoints[Index], CurrentLocation))
        {
            CurrentLocation = CachedTraceSourceComponent->GetComponentLocation();
        }

        PreviousTraceLocations[Index] = CurrentLocation;
    }

    SetComponentTickEnabled(true);
    OnHitWindowStarted.Broadcast(this, ActiveWindowName);

    return true;
}

bool UActionCombatMeleeTraceComponent::ShouldRunTraceAuthority() const
{
    if (!bServerAuthorityOnly)
    {
        return true;
    }

    const AActor* Owner = GetOwner();
    return Owner != nullptr && Owner->HasAuthority();
}

bool UActionCombatMeleeTraceComponent::GetTracePointWorldLocation(const FActionCombatTracePoint& TracePoint, FVector& OutLocation) const
{
    const USceneComponent* TraceComponent = ResolveTraceSourceComponent();
    if (!TraceComponent)
    {
        return false;
    }

    const FTransform BaseTransform = (!TracePoint.SocketName.IsNone() && TraceComponent->DoesSocketExist(TracePoint.SocketName))
        ? TraceComponent->GetSocketTransform(TracePoint.SocketName, RTS_World)
        : TraceComponent->GetComponentTransform();

    OutLocation = BaseTransform.TransformPosition(TracePoint.LocalOffset);
    return true;
}

bool UActionCombatMeleeTraceComponent::HasReachedUniqueTargetLimit() const
{
    return ActiveTraceProfile.MaxUniqueTargetsPerWindow > 0
        && HitActorsThisWindow.Num() >= ActiveTraceProfile.MaxUniqueTargetsPerWindow;
}

bool UActionCombatMeleeTraceComponent::WasActorAlreadyHit(const AActor* Actor) const
{
    return Actor != nullptr && HitActorsThisWindow.Contains(TObjectKey<AActor>(Actor));
}

void UActionCombatMeleeTraceComponent::RememberHitActor(AActor* Actor)
{
    if (Actor)
    {
        HitActorsThisWindow.Add(TObjectKey<AActor>(Actor));
    }
}

void UActionCombatMeleeTraceComponent::TraceActiveWindow()
{
    UWorld* World = GetWorld();
    AActor* Owner = GetOwner();
    USceneComponent* TraceSource = ResolveTraceSourceComponent();

    if (!World || !Owner || !TraceSource)
    {
        return;
    }

    for (int32 PointIndex = 0; PointIndex < ActiveTraceProfile.TracePoints.Num(); ++PointIndex)
    {
        if (HasReachedUniqueTargetLimit())
        {
            break;
        }

        FVector CurrentLocation = FVector::ZeroVector;
        if (!GetTracePointWorldLocation(ActiveTraceProfile.TracePoints[PointIndex], CurrentLocation))
        {
            CurrentLocation = TraceSource->GetComponentLocation();
        }

        const FVector StartLocation = PreviousTraceLocations.IsValidIndex(PointIndex) ? PreviousTraceLocations[PointIndex] : CurrentLocation;
        const FVector EndLocation = CurrentLocation;

        FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ActionCombatMeleeTrace), ActiveTraceProfile.bTraceComplex, Owner);
        QueryParams.bReturnPhysicalMaterial = true;
        QueryParams.AddIgnoredActor(Owner);

        if (ActiveTraceProfile.bIgnoreOwnerAttachedActors)
        {
            TArray<AActor*> AttachedActors;
            Owner->GetAttachedActors(AttachedActors);
            QueryParams.AddIgnoredActors(AttachedActors);
        }

        TArray<FHitResult> HitResults;
        if (ActiveTraceProfile.SweepRadius > KINDA_SMALL_NUMBER)
        {
            World->SweepMultiByChannel(
                HitResults,
                StartLocation,
                EndLocation,
                FQuat::Identity,
                ActiveTraceProfile.TraceChannel,
                FCollisionShape::MakeSphere(ActiveTraceProfile.SweepRadius),
                QueryParams);
        }
        else
        {
            World->LineTraceMultiByChannel(
                HitResults,
                StartLocation,
                EndLocation,
                ActiveTraceProfile.TraceChannel,
                QueryParams);
        }

        if (bDrawDebug)
        {
            DrawDebugLine(World, StartLocation, EndLocation, FColor::Orange, false, 1.0f, 0, 1.25f);
            DrawDebugSphere(World, EndLocation, ActiveTraceProfile.SweepRadius, 12, FColor::Orange, false, 1.0f);
        }

        int32 ProcessedHitCount = 0;
        for (const FHitResult& HitResult : HitResults)
        {
            if (ActiveTraceProfile.MaxHitResultsPerTick > 0 && ProcessedHitCount >= ActiveTraceProfile.MaxHitResultsPerTick)
            {
                break;
            }

            const bool bAcceptedHit = RecordHit(HitResult);
            ++ProcessedHitCount;

            if (ActiveTraceProfile.bStopAtBlockingHit && HitResult.bBlockingHit)
            {
                break;
            }

            if (bAcceptedHit && HasReachedUniqueTargetLimit())
            {
                break;
            }
        }

        PreviousTraceLocations[PointIndex] = EndLocation;
    }
}

bool UActionCombatMeleeTraceComponent::RecordHit(const FHitResult& HitResult)
{
    AActor* HitActor = HitResult.GetActor();
    if (!HitActor || HitActor == GetOwner())
    {
        return false;
    }

    if (ActiveTraceProfile.DedupeMode == EActionCombatMeleeHitDedupeMode::OncePerWindow && WasActorAlreadyHit(HitActor))
    {
        return false;
    }

    FActionCombatRecordedHit RecordedHit;
    RecordedHit.HitResult = HitResult;

    if (const UActionCombatHurtboxComponent* Hurtbox = Cast<UActionCombatHurtboxComponent>(HitResult.GetComponent()))
    {
        RecordedHit.HitZoneTag = Hurtbox->GetHitZoneTag();
        RecordedHit.DamageMultiplier = Hurtbox->GetDamageMultiplier();
    }

    const int32 HitIndex = RecordedHits.Add(RecordedHit);
    if (ActiveTraceProfile.DedupeMode == EActionCombatMeleeHitDedupeMode::OncePerWindow)
    {
        RememberHitActor(HitActor);
    }

    OnRecordedHit.Broadcast(this, RecordedHit, HitIndex);
    return true;
}
