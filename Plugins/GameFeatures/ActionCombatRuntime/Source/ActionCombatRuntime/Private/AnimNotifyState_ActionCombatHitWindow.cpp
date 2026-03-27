#include "AnimNotifyState_ActionCombatHitWindow.h"

#include "ActionCombatBlueprintLibrary.h"
#include "ActionCombatMeleeTraceComponent.h"
#include "ActionCombatRuntimeLog.h"

#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"

void UAnimNotifyState_ActionCombatHitWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

    AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
    if (!Owner)
    {
        return;
    }

    TArray<UActionCombatMeleeTraceComponent*> TraceComponents = UActionCombatBlueprintLibrary::FindMeleeTraceComponents(Owner, TraceSourceId, true);
    if (TraceComponents.Num() == 0)
    {
        UE_LOG(LogActionCombatRuntime, Verbose, TEXT("No melee trace components found for hit window '%s' on %s."), *WindowName.ToString(), *GetNameSafe(Owner));
        return;
    }

    for (UActionCombatMeleeTraceComponent* TraceComponent : TraceComponents)
    {
        if (!TraceComponent)
        {
            continue;
        }

        if (bUseOverrideProfile)
        {
            TraceComponent->StartHitWindowWithProfile(WindowName, OverrideTraceProfile);
        }
        else
        {
            TraceComponent->StartHitWindowWithDefaultProfile(WindowName);
        }
    }
}

void UAnimNotifyState_ActionCombatHitWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyEnd(MeshComp, Animation, EventReference);

    AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
    if (!Owner)
    {
        return;
    }

    TArray<UActionCombatMeleeTraceComponent*> TraceComponents = UActionCombatBlueprintLibrary::FindMeleeTraceComponents(Owner, TraceSourceId, true);
    for (UActionCombatMeleeTraceComponent* TraceComponent : TraceComponents)
    {
        if (TraceComponent)
        {
            TraceComponent->StopHitWindow();
        }
    }
}

FString UAnimNotifyState_ActionCombatHitWindow::GetNotifyName_Implementation() const
{
    return WindowName.IsNone()
        ? TEXT("ActionCombat Hit Window")
        : FString::Printf(TEXT("ActionCombat Hit Window (%s)"), *WindowName.ToString());
}
