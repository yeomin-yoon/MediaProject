#include "ActionCombatLyraMeleeWeaponSpawnerComponent.h"

#include "ActionCombatMeleeWeaponActor.h"
#include "ActionCombatRuntimeLog.h"

#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"

UActionCombatLyraMeleeWeaponSpawnerComponent::UActionCombatLyraMeleeWeaponSpawnerComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    PrimaryComponentTick.bCanEverTick = false;
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UActionCombatLyraMeleeWeaponSpawnerComponent::BeginPlay()
{
    Super::BeginPlay();

    if (bSpawnOnBeginPlay)
    {
        SpawnWeapon();
    }
}

void UActionCombatLyraMeleeWeaponSpawnerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (bDestroyWeaponOnEndPlay)
    {
        DestroyWeapon();
    }

    Super::EndPlay(EndPlayReason);
}

AActionCombatMeleeWeaponActor* UActionCombatLyraMeleeWeaponSpawnerComponent::SpawnWeapon()
{
    AActor* Owner = GetOwner();
    if ((Owner == nullptr) || !Owner->HasAuthority())
    {
        return SpawnedWeaponActor.Get();
    }

    if (SpawnedWeaponActor.Get() != nullptr)
    {
        return SpawnedWeaponActor.Get();
    }

    if (WeaponActorClass == nullptr)
    {
        UE_LOG(LogActionCombatRuntime, Warning, TEXT("[LyraMeleeWeaponSpawner:%s] Spawn skipped because WeaponActorClass was not set."), *GetPathNameSafe(Owner));
        return nullptr;
    }

    UWorld* World = GetWorld();
    USceneComponent* AttachParent = ResolveAttachParent();
    if ((World == nullptr) || (AttachParent == nullptr))
    {
        UE_LOG(LogActionCombatRuntime, Warning, TEXT("[LyraMeleeWeaponSpawner:%s] Spawn skipped because AttachParent could not be resolved."), *GetPathNameSafe(Owner));
        return nullptr;
    }

    FActorSpawnParameters SpawnParameters;
    SpawnParameters.Owner = Owner;
    SpawnParameters.Instigator = Cast<APawn>(Owner);
    SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    SpawnedWeaponActor = World->SpawnActor<AActionCombatMeleeWeaponActor>(WeaponActorClass, SpawnParameters);
    if (SpawnedWeaponActor == nullptr)
    {
        return nullptr;
    }

    SpawnedWeaponActor->AttachToComponent(AttachParent, FAttachmentTransformRules::SnapToTargetNotIncludingScale, AttachSocket);
    SpawnedWeaponActor->SetActorRelativeTransform(RelativeAttachTransform);
    return SpawnedWeaponActor.Get();
}

void UActionCombatLyraMeleeWeaponSpawnerComponent::DestroyWeapon()
{
    AActor* Owner = GetOwner();
    if ((Owner == nullptr) || !Owner->HasAuthority())
    {
        return;
    }

    if (AActionCombatMeleeWeaponActor* WeaponActor = SpawnedWeaponActor.Get())
    {
        WeaponActor->Destroy();
    }

    SpawnedWeaponActor = nullptr;
}

AActionCombatMeleeWeaponActor* UActionCombatLyraMeleeWeaponSpawnerComponent::GetSpawnedWeapon() const
{
    if (SpawnedWeaponActor.Get() != nullptr)
    {
        return SpawnedWeaponActor.Get();
    }

    AActor* Owner = GetOwner();
    if (Owner == nullptr)
    {
        return nullptr;
    }

    TArray<AActor*> AttachedActors;
    Owner->GetAttachedActors(AttachedActors);
    for (AActor* AttachedActor : AttachedActors)
    {
        if (AActionCombatMeleeWeaponActor* WeaponActor = Cast<AActionCombatMeleeWeaponActor>(AttachedActor))
        {
            return WeaponActor;
        }
    }

    return nullptr;
}

USceneComponent* UActionCombatLyraMeleeWeaponSpawnerComponent::ResolveAttachParent() const
{
    AActor* Owner = GetOwner();
    if (Owner == nullptr)
    {
        return nullptr;
    }

    if (USceneComponent* ExplicitComponent = Cast<USceneComponent>(AttachParentComponent.GetComponent(Owner)))
    {
        return ExplicitComponent;
    }

    if (USkeletalMeshComponent* MeshComponent = Owner->FindComponentByClass<USkeletalMeshComponent>())
    {
        return MeshComponent;
    }

    return Owner->GetRootComponent();
}
