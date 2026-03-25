#include "ActionCombatHurtboxComponent.h"

UActionCombatHurtboxComponent::UActionCombatHurtboxComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    SetGenerateOverlapEvents(false);
    SetCollisionObjectType(ECC_WorldDynamic);
    SetCollisionResponseToAllChannels(ECR_Ignore);
    SetHiddenInGame(true);
    bUseAttachParentBound = true;
}

void UActionCombatHurtboxComponent::OnRegister()
{
    Super::OnRegister();

    if (bAutoConfigureForLyraWeaponTraces)
    {
        ConfigureForLyraWeaponTraces();
    }
}

void UActionCombatHurtboxComponent::ConfigureForLyraWeaponTraces()
{
    SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Overlap);
    SetCollisionResponseToChannel(ECC_GameTraceChannel3, ECR_Overlap);
    SetCollisionResponseToChannel(ECC_GameTraceChannel4, ECR_Overlap);
}
