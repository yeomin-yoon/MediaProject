// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameFramework/Actor.h"
#include "Interaction/IInteractableTarget.h"
#include "Interaction/InteractionOption.h"
#include "Inventory/IPickupable.h"
#include "Inventory/LyraInventoryItemInstance.h"
#include "LyraWorldCollectable.generated.h"

class UBoxComponent;
class UNiagaraComponent;
class UNiagaraSystem;
class UObject;
class UProjectileMovementComponent;
struct FInteractionQuery;

/**
 * 
 */
UCLASS(Abstract, Blueprintable)
class SHOOTERCORERUNTIME_API ALyraWorldCollectable : public AActor, public IInteractableTarget, public IPickupable
{
	GENERATED_BODY()

public:

	ALyraWorldCollectable();

	virtual void GatherInteractionOptions(const FInteractionQuery& InteractQuery, FInteractionOptionBuilder& InteractionBuilder) override;
	virtual FInventoryPickup GetPickupInventory() const override;

protected:
	UPROPERTY(EditAnywhere)
	FInteractionOption Option;
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<UBoxComponent> BoxComp;
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<UNiagaraComponent> NiagaraComp;

	UPROPERTY()
	TObjectPtr<UNiagaraSystem> RedNS;

	UPROPERTY()
	TObjectPtr<UNiagaraSystem> YellowNS;

	UPROPERTY()
	TObjectPtr<UNiagaraSystem> BlueNS;
	
public:
	UPROPERTY(EditAnywhere)
	FInventoryPickup StaticInventory;
	
	void LaunchItem(FVector Velocity);

	UPROPERTY(EditAnywhere)
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;
	
	UPROPERTY(BlueprintReadOnly)
	EItemOptionType OptionType = EItemOptionType::Attack;

	UPROPERTY(BlueprintReadOnly)
	int32 RandomSeed = 0;
	
	UPROPERTY(BlueprintReadOnly)
	EItemRarity Rarity = EItemRarity::Common;
	
	void ApplyNiagaraByOption();
};
