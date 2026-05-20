// Copyright Epic Games, Inc. All Rights Reserved.

#include "IPickupable.h"

#include "LyraInventoryItemDefinition.h"
#include "GameFramework/Actor.h"
#include "LyraInventoryManagerComponent.h"
#include "UObject/ScriptInterface.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(IPickupable)

class UActorComponent;

UPickupableStatics::UPickupableStatics()
	: Super(FObjectInitializer::Get())
{
}

TScriptInterface<IPickupable> UPickupableStatics::GetFirstPickupableFromActor(AActor* Actor)
{
	// If the actor is directly pickupable, return that.
	TScriptInterface<IPickupable> PickupableActor(Actor);
	if (PickupableActor)
	{
		return PickupableActor;
	}

	// If the actor isn't pickupable, it might have a component that has a pickupable interface.
	TArray<UActorComponent*> PickupableComponents = Actor ? Actor->GetComponentsByInterface(UPickupable::StaticClass()) : TArray<UActorComponent*>();
	if (PickupableComponents.Num() > 0)
	{
		// Get first pickupable, if the user needs more sophisticated pickup distinction, will need to be solved elsewhere.
		return TScriptInterface<IPickupable>(PickupableComponents[0]);
	}

	return TScriptInterface<IPickupable>();
}

void UPickupableStatics::AddPickupToInventory(
	ULyraInventoryManagerComponent* InventoryComponent,
	TScriptInterface<IPickupable> Pickup)
{
	if (!InventoryComponent || !Pickup)
		return;

	const FInventoryPickup& PickupInventory =
		Pickup->GetPickupInventory();

	// =========================
	// Templates
	// =========================
	for (const FPickupTemplate& Template : PickupInventory.Templates)
	{
		if (!Template.ItemDef)
			continue;

		InventoryComponent->AddItemDefinition(
			Template.ItemDef,
			Template.StackCount,
			Template.RandomSeed,
			Template.OptionType,
			Template.Rarity
		);
	}

	// =========================
	// Instances
	// =========================
	for (const FPickupInstance& Instance : PickupInventory.Instances)
	{
		if (!Instance.Item)
			continue;
		
		InventoryComponent->AddItemInstance(Instance.Item);
	}
}