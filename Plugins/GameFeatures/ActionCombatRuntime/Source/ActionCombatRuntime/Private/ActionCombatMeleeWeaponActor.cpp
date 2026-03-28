#include "ActionCombatMeleeWeaponActor.h"

#include "ActionCombatMeleeTraceComponent.h"

#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"

AActionCombatMeleeWeaponActor::AActionCombatMeleeWeaponActor(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    bReplicates = true;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
    WeaponMesh->SetupAttachment(SceneRoot);
    WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    WeaponMesh->SetGenerateOverlapEvents(false);

    MeleeTraceComponent = CreateDefaultSubobject<UActionCombatMeleeTraceComponent>(TEXT("MeleeTraceComponent"));
}
