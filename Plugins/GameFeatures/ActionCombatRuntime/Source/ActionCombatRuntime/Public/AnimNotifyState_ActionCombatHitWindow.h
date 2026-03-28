#pragma once

#include "Animation/AnimNotifies/AnimNotifyState.h"

#include "ActionCombatMeleeTraceTypes.h"
#include "AnimNotifyState_ActionCombatHitWindow.generated.h"

class UAnimSequenceBase;
class USkeletalMeshComponent;

UCLASS(meta = (DisplayName = "Action Combat Hit Window"))
class ACTIONCOMBATRUNTIME_API UAnimNotifyState_ActionCombatHitWindow : public UAnimNotifyState
{
    GENERATED_BODY()

public:
    virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
    virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
    virtual FString GetNotifyName_Implementation() const override;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Window")
    FName TraceSourceId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Window")
    FName WindowName = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Window")
    bool bUseOverrideProfile = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Window", meta = (EditCondition = "bUseOverrideProfile"))
    FActionCombatMeleeTraceProfile OverrideTraceProfile;
};
