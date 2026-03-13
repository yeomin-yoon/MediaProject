#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "LockOnTargetComponent.generated.h"

class APawn;

UCLASS(ClassGroup = (LockOn), BlueprintType, meta = (BlueprintSpawnableComponent))
class LOCKONSYSTEMRUNTIME_API ULockOnTargetComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    ULockOnTargetComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UFUNCTION(BlueprintCallable, Category = "LockOn")
    bool CanBeLockedOnBy(const APawn* RequestingPawn) const;

    UFUNCTION(BlueprintCallable, Category = "LockOn")
    FVector GetLockOnFocusLocation() const;

    UFUNCTION(BlueprintPure, Category = "LockOn")
    float GetTargetPriorityBonus() const { return TargetPriorityBonus; }

    UFUNCTION(BlueprintPure, Category = "LockOn")
    bool UsesLargeBossCameraProfile() const { return bUseLargeBossCameraProfile; }

private:
    void RegisterWithSubsystem();
    void UnregisterFromSubsystem();

private:
    UPROPERTY(EditAnywhere, Category = "LockOn")
    bool bLockOnEnabled = true;

    UPROPERTY(EditAnywhere, Category = "LockOn")
    float TargetPriorityBonus = 0.0f;

    UPROPERTY(EditAnywhere, Category = "LockOn")
    FName TargetSocketName;

    UPROPERTY(EditAnywhere, Category = "LockOn")
    FVector TargetLocationOffset = FVector(0.0f, 0.0f, 75.0f);

    UPROPERTY(EditAnywhere, Category = "LockOn")
    bool bUseLargeBossCameraProfile = false;

    bool bIsRegisteredWithSubsystem = false;
    FIntPoint RegisteredCellCoord = FIntPoint::ZeroValue;
};
