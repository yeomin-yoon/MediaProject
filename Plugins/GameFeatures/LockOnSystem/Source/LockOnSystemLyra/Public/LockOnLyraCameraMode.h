#pragma once

#include "CoreMinimal.h"
#include "Camera/LyraCameraMode.h"

#include "LockOnLyraCameraMode.generated.h"

UCLASS()
class LOCKONSYSTEMLYRA_API ULockOnLyraCameraMode : public ULyraCameraMode
{
	GENERATED_BODY()

protected:
	virtual void UpdateView(float DeltaTime) override;

private:
	FVector ResolveCameraLocation(const FVector& PivotLocation, const FRotator& PivotRotation, const FVector& CameraOffset, const AActor* OwningActor, const AActor* TargetActor) const;

private:
	UPROPERTY(EditDefaultsOnly, Category = "LockOn")
	FVector BaseCameraLocalOffset = FVector(-280.0f, 90.0f, 45.0f);

	UPROPERTY(EditDefaultsOnly, Category = "LockOn")
	bool bEnableCameraCollision = true;

	UPROPERTY(EditDefaultsOnly, Category = "LockOn", meta = (EditCondition = "bEnableCameraCollision", ClampMin = "1.0"))
	float CollisionProbeRadius = 12.0f;
};
