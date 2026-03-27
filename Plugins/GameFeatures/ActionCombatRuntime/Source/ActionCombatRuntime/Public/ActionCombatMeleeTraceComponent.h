#pragma once

#include "Abilities/GameplayAbilityTargetTypes.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "UObject/ObjectKey.h"

#include "ActionCombatMeleeTraceTypes.h"
#include "ActionCombatMeleeTraceComponent.generated.h"

class AActor;
class USceneComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FActionCombatHitWindowSignature, UActionCombatMeleeTraceComponent*, TraceComponent, FName, WindowName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FActionCombatRecordedHitSignature, UActionCombatMeleeTraceComponent*, TraceComponent, FActionCombatRecordedHit, RecordedHit, int32, HitIndex);

UCLASS(BlueprintType, ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class ACTIONCOMBATRUNTIME_API UActionCombatMeleeTraceComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UActionCombatMeleeTraceComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UFUNCTION(BlueprintCallable, Category = "Action Combat|Melee Trace")
    bool StartHitWindowWithDefaultProfile(FName WindowName = NAME_None);

    UFUNCTION(BlueprintCallable, Category = "Action Combat|Melee Trace")
    bool StartHitWindowWithProfile(FName WindowName, const FActionCombatMeleeTraceProfile& TraceProfile);

    UFUNCTION(BlueprintCallable, Category = "Action Combat|Melee Trace")
    void StopHitWindow();

    UFUNCTION(BlueprintPure, Category = "Action Combat|Melee Trace")
    bool IsHitWindowActive() const
    {
        return bHitWindowActive;
    }

    UFUNCTION(BlueprintPure, Category = "Action Combat|Melee Trace")
    FName GetTraceSourceId() const
    {
        return TraceSourceId;
    }

    UFUNCTION(BlueprintPure, Category = "Action Combat|Melee Trace")
    FName GetActiveWindowName() const
    {
        return ActiveWindowName;
    }

    UFUNCTION(BlueprintCallable, Category = "Action Combat|Melee Trace")
    void SetDefaultTraceProfile(const FActionCombatMeleeTraceProfile& NewProfile);

    UFUNCTION(BlueprintPure, Category = "Action Combat|Melee Trace")
    USceneComponent* ResolveTraceSourceComponent() const;

    UFUNCTION(BlueprintCallable, Category = "Action Combat|Melee Trace")
    void ClearRecordedHits();

    UFUNCTION(BlueprintCallable, Category = "Action Combat|Melee Trace")
    void ConsumeRecordedHits(TArray<FActionCombatRecordedHit>& OutHits);

    UFUNCTION(BlueprintCallable, Category = "Action Combat|Melee Trace")
    void ConsumeRecordedHitResults(TArray<FHitResult>& OutHitResults);

    UFUNCTION(BlueprintCallable, Category = "Action Combat|Melee Trace")
    FGameplayAbilityTargetDataHandle ConsumeRecordedTargetData();

    UPROPERTY(BlueprintAssignable, Category = "Action Combat|Melee Trace")
    FActionCombatHitWindowSignature OnHitWindowStarted;

    UPROPERTY(BlueprintAssignable, Category = "Action Combat|Melee Trace")
    FActionCombatHitWindowSignature OnHitWindowStopped;

    UPROPERTY(BlueprintAssignable, Category = "Action Combat|Melee Trace")
    FActionCombatRecordedHitSignature OnRecordedHit;

protected:
    virtual void BeginPlay() override;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trace")
    FName TraceSourceId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trace")
    FComponentReference TraceSourceComponent;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trace")
    FActionCombatMeleeTraceProfile DefaultTraceProfile;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trace")
    bool bServerAuthorityOnly = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trace")
    bool bDrawDebug = false;

private:
    bool StartHitWindowInternal(FName WindowName, const FActionCombatMeleeTraceProfile& TraceProfile);
    bool ShouldRunTraceAuthority() const;
    bool GetTracePointWorldLocation(const FActionCombatTracePoint& TracePoint, FVector& OutLocation) const;
    bool HasReachedUniqueTargetLimit() const;
    bool WasActorAlreadyHit(const AActor* Actor) const;
    void RememberHitActor(AActor* Actor);
    void TraceActiveWindow();
    bool RecordHit(const FHitResult& HitResult);

    FActionCombatMeleeTraceProfile ActiveTraceProfile;
    TWeakObjectPtr<USceneComponent> CachedTraceSourceComponent;
    TArray<FVector> PreviousTraceLocations;
    TArray<FActionCombatRecordedHit> RecordedHits;
    TSet<TObjectKey<AActor>> HitActorsThisWindow;
    FName ActiveWindowName = NAME_None;
    bool bHitWindowActive = false;
};
