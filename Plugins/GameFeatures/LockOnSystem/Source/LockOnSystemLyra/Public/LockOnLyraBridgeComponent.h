#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Camera/LyraCameraComponent.h"

#include "LockOnLyraBridgeComponent.generated.h"

class ULockOnComponent;
class ULockOnLyraCameraMode;
class UInputComponent;
struct FInputActionValue;

UCLASS(ClassGroup = (LockOn), meta = (BlueprintSpawnableComponent))
class LOCKONSYSTEMLYRA_API ULockOnLyraBridgeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULockOnLyraBridgeComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void EnsureDelegateChain();
	void ReleaseDelegateChain();
	void TryBindInput();
	void ProcessMouseCycle(float DeltaTime);
	bool ShouldManageCamera() const;
	TSubclassOf<ULyraCameraMode> DetermineCameraModeFromBridge() const;
	void HandleToggleLockPressed();
	void HandleLookMouse(const FInputActionValue& InputActionValue);

private:
	TObjectPtr<ULockOnComponent> CachedLockOnComponent;
	TWeakObjectPtr<ULyraCameraComponent> CachedCameraComponent;
	TWeakObjectPtr<UInputComponent> BoundInputComponent;
	FLyraCameraModeDelegate BaseCameraModeDelegate;
	bool bHasBoundDelegateOnce = false;
	FVector2D PendingMouseCycleInput = FVector2D::ZeroVector;
	float MouseCycleCooldownRemaining = 0.0f;
	float MouseCycleIdleSeconds = 0.0f;

	UPROPERTY(EditAnywhere, Category = "LockOn")
	TSubclassOf<ULyraCameraMode> LockOnCameraModeClass;
};
