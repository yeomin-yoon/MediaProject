#include "LockOnTargetComponent.h"

#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "LockOnWorldSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LockOnTargetComponent)

ULockOnTargetComponent::ULockOnTargetComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
}

void ULockOnTargetComponent::BeginPlay()
{
    Super::BeginPlay();
    RegisterWithSubsystem();
}

void ULockOnTargetComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    UnregisterFromSubsystem();
    Super::EndPlay(EndPlayReason);
}

void ULockOnTargetComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!bIsRegisteredWithSubsystem)
    {
        RegisterWithSubsystem();
        return;
    }

    if (ULockOnWorldSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<ULockOnWorldSubsystem>() : nullptr)
    {
        const FIntPoint NewCellCoord = Subsystem->ComputeCellCoord(GetOwner()->GetActorLocation());
        if (NewCellCoord != RegisteredCellCoord)
        {
            Subsystem->MoveTarget(this, RegisteredCellCoord, NewCellCoord);
            RegisteredCellCoord = NewCellCoord;
        }
    }
}

bool ULockOnTargetComponent::CanBeLockedOnBy(const APawn* RequestingPawn) const
{
    const AActor* OwnerActor = GetOwner();
    return bLockOnEnabled && IsValid(OwnerActor) && !OwnerActor->IsHidden() && (OwnerActor != RequestingPawn);
}

FVector ULockOnTargetComponent::GetLockOnFocusLocation() const
{
    const AActor* OwnerActor = GetOwner();
    if (!IsValid(OwnerActor))
    {
        return FVector::ZeroVector;
    }

    if (!TargetSocketName.IsNone())
    {
        if (const USceneComponent* RootComponent = OwnerActor->GetRootComponent())
        {
            if (const USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(RootComponent))
            {
                if (SkeletalMeshComponent->DoesSocketExist(TargetSocketName))
                {
                    return SkeletalMeshComponent->GetSocketLocation(TargetSocketName) + TargetLocationOffset;
                }
            }
        }

        TInlineComponentArray<USkeletalMeshComponent*> SkeletalMeshComponents(OwnerActor);
        for (const USkeletalMeshComponent* SkeletalMeshComponent : SkeletalMeshComponents)
        {
            if (SkeletalMeshComponent && SkeletalMeshComponent->DoesSocketExist(TargetSocketName))
            {
                return SkeletalMeshComponent->GetSocketLocation(TargetSocketName) + TargetLocationOffset;
            }
        }
    }

    return OwnerActor->GetActorLocation() + TargetLocationOffset;
}

void ULockOnTargetComponent::RegisterWithSubsystem()
{
    if (bIsRegisteredWithSubsystem || !GetWorld() || !GetOwner())
    {
        return;
    }

    if (ULockOnWorldSubsystem* Subsystem = GetWorld()->GetSubsystem<ULockOnWorldSubsystem>())
    {
        RegisteredCellCoord = Subsystem->ComputeCellCoord(GetOwner()->GetActorLocation());
        Subsystem->RegisterTarget(this, RegisteredCellCoord);
        bIsRegisteredWithSubsystem = true;
    }
}

void ULockOnTargetComponent::UnregisterFromSubsystem()
{
    if (!bIsRegisteredWithSubsystem || !GetWorld())
    {
        return;
    }

    if (ULockOnWorldSubsystem* Subsystem = GetWorld()->GetSubsystem<ULockOnWorldSubsystem>())
    {
        Subsystem->UnregisterTarget(this, RegisteredCellCoord);
    }

    bIsRegisteredWithSubsystem = false;
    RegisteredCellCoord = FIntPoint::ZeroValue;
}
