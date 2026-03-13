#include "LockOnLyraCameraMode.h"

#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Engine/World.h"
#include "LockOnComponent.h"
#include "LockOnSettings.h"
#include "LockOnTargetComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LockOnLyraCameraMode)

void ULockOnLyraCameraMode::UpdateView(float DeltaTime)
{
	const AActor* OwningActor = GetTargetActor();
	const ULockOnComponent* LockOnComponent = OwningActor ? OwningActor->FindComponentByClass<ULockOnComponent>() : nullptr;
	FVector PivotLocation = GetPivotLocation();
	FRotator PivotRotation = GetPivotRotation();

	FVector FocusLocation = FVector::ZeroVector;
	const bool bHasActiveLock = LockOnComponent && LockOnComponent->IsLockActive() && LockOnComponent->GetCurrentTargetFocusLocation(FocusLocation);
	bool bUseLargeBossProfile = false;
	AActor* CurrentTargetActor = LockOnComponent ? LockOnComponent->GetCurrentTargetActor() : nullptr;
	if (CurrentTargetActor)
	{
		if (const ULockOnTargetComponent* TargetComponent = CurrentTargetActor->FindComponentByClass<ULockOnTargetComponent>())
		{
			bUseLargeBossProfile = TargetComponent->UsesLargeBossCameraProfile();
		}
	}

	const FLockOnCameraProfile& CameraProfile = ULockOnSettings::Get()->GetCameraProfile(bUseLargeBossProfile);
	if (bHasActiveLock)
	{
		const FRotator DesiredRotation = (FocusLocation - PivotLocation).Rotation();
		PivotRotation.Yaw = FMath::FixedTurn(PivotRotation.Yaw, DesiredRotation.Yaw, CameraProfile.YawInterpSpeedDegPerSecond * DeltaTime);
		PivotRotation.Pitch = FMath::FixedTurn(PivotRotation.Pitch, DesiredRotation.Pitch + CameraProfile.PitchBiasDegrees, CameraProfile.PitchInterpSpeedDegPerSecond * DeltaTime);
	}

	PivotRotation.Pitch = FMath::ClampAngle(PivotRotation.Pitch, ViewPitchMin, ViewPitchMax);

	View.Rotation = PivotRotation;
	View.ControlRotation = View.Rotation;
	View.FieldOfView = FieldOfView + (bHasActiveLock ? CameraProfile.FieldOfViewOffset : 0.0f);

	const FVector CameraOffset = BaseCameraLocalOffset + (bHasActiveLock ? CameraProfile.AdditionalTargetOffset : FVector::ZeroVector);
	View.Location = ResolveCameraLocation(PivotLocation, PivotRotation, CameraOffset, OwningActor, CurrentTargetActor);
}

FVector ULockOnLyraCameraMode::ResolveCameraLocation(const FVector& PivotLocation, const FRotator& PivotRotation, const FVector& CameraOffset, const AActor* OwningActor, const AActor* TargetActor) const
{
	const FVector DesiredLocation = PivotLocation + PivotRotation.RotateVector(CameraOffset);
	if (!bEnableCameraCollision)
	{
		return DesiredLocation;
	}

	UWorld* World = GetWorld();
	if (!World || !OwningActor)
	{
		return DesiredLocation;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(LockOnCameraSweep), false);
	QueryParams.AddIgnoredActor(OwningActor);
	if (TargetActor)
	{
		QueryParams.AddIgnoredActor(TargetActor);
	}

	const FVector SweepDelta = DesiredLocation - PivotLocation;
	if (SweepDelta.IsNearlyZero())
	{
		return DesiredLocation;
	}

	FHitResult Hit;
	const bool bBlockingHit = World->SweepSingleByChannel(
		Hit,
		PivotLocation,
		DesiredLocation,
		FQuat::Identity,
		ECC_Camera,
		FCollisionShape::MakeSphere(CollisionProbeRadius),
		QueryParams);

	if (!bBlockingHit)
	{
		return DesiredLocation;
	}

	const float SafeTime = FMath::Clamp(Hit.Time - 0.03f, 0.0f, 1.0f);
	return PivotLocation + (SweepDelta * SafeTime);
}
