// Copyright Epic Games, Inc. All Rights Reserved.

#include "LyraWorldCollectable.h"
#include "Async/TaskGraphInterfaces.h"
#include "GameFramework/ProjectileMovementComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraWorldCollectable)

struct FInteractionQuery;

ALyraWorldCollectable::ALyraWorldCollectable()
{
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>("ProjectileMovement");
	
	ProjectileMovement->InitialSpeed = 0.f;
	ProjectileMovement->MaxSpeed = 2000.f;

	ProjectileMovement->ProjectileGravityScale = 2.0f;

	ProjectileMovement->bRotationFollowsVelocity = false;
	ProjectileMovement->bShouldBounce = true;

	ProjectileMovement->Bounciness = 0.35f;
	ProjectileMovement->Friction = 0.6f;
	ProjectileMovement->BounceVelocityStopSimulatingThreshold = 60.f;
}

void ALyraWorldCollectable::GatherInteractionOptions(const FInteractionQuery& InteractQuery, FInteractionOptionBuilder& InteractionBuilder)
{
	InteractionBuilder.AddInteractionOption(Option);
}

FInventoryPickup ALyraWorldCollectable::GetPickupInventory() const
{
	return StaticInventory;
}

void ALyraWorldCollectable::LaunchItem(FVector Velocity)
{
	if (ProjectileMovement)
	{
		ProjectileMovement->StopMovementImmediately();
		ProjectileMovement->Velocity = Velocity;
		ProjectileMovement->UpdateComponentVelocity();
		ProjectileMovement->Activate(true);
	}
}