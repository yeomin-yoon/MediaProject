#include "AbilityTask_WaitActionCombatMeleeHit.h"

#include "ActionCombatBlueprintLibrary.h"
#include "ActionCombatMeleeTraceComponent.h"

#include "GameFramework/Actor.h"

UAbilityTask_WaitActionCombatMeleeHit* UAbilityTask_WaitActionCombatMeleeHit::WaitActionCombatMeleeHit(UGameplayAbility* OwningAbility, FName TraceSourceId, bool bIncludeAttachedActors, bool bTriggerOnce, bool bEndTaskIfNoTraceComponents)
{
    UAbilityTask_WaitActionCombatMeleeHit* Task = NewAbilityTask<UAbilityTask_WaitActionCombatMeleeHit>(OwningAbility);
    Task->RequestedTraceSourceId = TraceSourceId;
    Task->bRequestedIncludeAttachedActors = bIncludeAttachedActors;
    Task->bRequestedTriggerOnce = bTriggerOnce;
    Task->bRequestedEndTaskIfNoTraceComponents = bEndTaskIfNoTraceComponents;
    return Task;
}

void UAbilityTask_WaitActionCombatMeleeHit::Activate()
{
    Super::Activate();
    BindToTraceComponents();

    if (BoundTraceComponents.IsEmpty() && bRequestedEndTaskIfNoTraceComponents)
    {
        EndTask();
    }
}

void UAbilityTask_WaitActionCombatMeleeHit::OnDestroy(bool AbilityEnded)
{
    UnbindFromTraceComponents();
    Super::OnDestroy(AbilityEnded);
}

void UAbilityTask_WaitActionCombatMeleeHit::BindToTraceComponents()
{
    BoundTraceComponents.Reset();

    AActor* AvatarActor = Ability ? Ability->GetAvatarActorFromActorInfo() : nullptr;
    if (!AvatarActor)
    {
        return;
    }

    const TArray<UActionCombatMeleeTraceComponent*> TraceComponents = UActionCombatBlueprintLibrary::FindMeleeTraceComponents(AvatarActor, RequestedTraceSourceId, bRequestedIncludeAttachedActors);
    for (UActionCombatMeleeTraceComponent* TraceComponent : TraceComponents)
    {
        if (!TraceComponent)
        {
            continue;
        }

        TraceComponent->OnRecordedHit.AddDynamic(this, &ThisClass::HandleRecordedHit);
        BoundTraceComponents.Add(TraceComponent);
    }
}

void UAbilityTask_WaitActionCombatMeleeHit::UnbindFromTraceComponents()
{
    for (UActionCombatMeleeTraceComponent* TraceComponent : BoundTraceComponents)
    {
        if (TraceComponent)
        {
            TraceComponent->OnRecordedHit.RemoveDynamic(this, &ThisClass::HandleRecordedHit);
        }
    }

    BoundTraceComponents.Reset();
}

void UAbilityTask_WaitActionCombatMeleeHit::HandleRecordedHit(UActionCombatMeleeTraceComponent* TraceComponent, FActionCombatRecordedHit RecordedHit, int32 HitIndex)
{
    if (ShouldBroadcastAbilityTaskDelegates())
    {
        OnHit.Broadcast(TraceComponent, RecordedHit, HitIndex);
    }

    if (bRequestedTriggerOnce)
    {
        EndTask();
    }
}
