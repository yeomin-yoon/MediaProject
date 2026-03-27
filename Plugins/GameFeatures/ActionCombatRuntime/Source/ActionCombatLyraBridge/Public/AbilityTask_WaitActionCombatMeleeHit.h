#pragma once

#include "Abilities/Tasks/AbilityTask.h"

#include "ActionCombatMeleeTraceTypes.h"
#include "AbilityTask_WaitActionCombatMeleeHit.generated.h"

class UActionCombatMeleeTraceComponent;
class UGameplayAbility;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FActionCombatAbilityTaskRecordedHitSignature, UActionCombatMeleeTraceComponent*, TraceComponent, FActionCombatRecordedHit, RecordedHit, int32, HitIndex);

UCLASS()
class ACTIONCOMBATLYRABRIDGE_API UAbilityTask_WaitActionCombatMeleeHit : public UAbilityTask
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable)
    FActionCombatAbilityTaskRecordedHitSignature OnHit;

    UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (DisplayName = "Wait Action Combat Melee Hit", HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "true"))
    static UAbilityTask_WaitActionCombatMeleeHit* WaitActionCombatMeleeHit(UGameplayAbility* OwningAbility, FName TraceSourceId = NAME_None, bool bIncludeAttachedActors = true, bool bTriggerOnce = false, bool bEndTaskIfNoTraceComponents = true);

    virtual void Activate() override;
    virtual void OnDestroy(bool AbilityEnded) override;

private:
    void BindToTraceComponents();
    void UnbindFromTraceComponents();

    UFUNCTION()
    void HandleRecordedHit(UActionCombatMeleeTraceComponent* TraceComponent, FActionCombatRecordedHit RecordedHit, int32 HitIndex);

    UPROPERTY()
    TArray<TObjectPtr<UActionCombatMeleeTraceComponent>> BoundTraceComponents;

    FName RequestedTraceSourceId = NAME_None;
    bool bRequestedIncludeAttachedActors = true;
    bool bRequestedTriggerOnce = false;
    bool bRequestedEndTaskIfNoTraceComponents = true;
};
