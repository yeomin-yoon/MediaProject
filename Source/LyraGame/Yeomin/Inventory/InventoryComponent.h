// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InventoryComponent.generated.h"

class UItemInstance;

USTRUCT(BlueprintType)
struct FInventorySlot
{
	GENERATED_BODY()
	
	UPROPERTY()
	TObjectPtr<UItemInstance> Item = nullptr;

	UPROPERTY()
	bool bOccupied = false;
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LYRAGAME_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInventoryComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Inventory")
	int32 GridWidth = 5;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Inventory")
	int32 GridHeight = 5;

	/* Slots */
	UPROPERTY()
	TArray<FInventorySlot> Slots;

	/* Items */
	UPROPERTY()
	TArray<TObjectPtr<UItemInstance>> Items;
	
public:
	/* Initialization */
	void InitializeInventory(int32 Width, int32 Height);

	/* Slot Helpers */
	int32 GetIndex(int32 X, int32 Y) const;
	bool IsValidPosition(int32 X, int32 Y) const;

	/* Item Placement */
	bool CanPlaceItem(UItemInstance* Item, int32 X, int32 Y);
	bool PlaceItem(UItemInstance* Item, int32 X, int32 Y);
	void RemoveItem(UItemInstance* Item);
	bool MoveItem(UItemInstance* Item, int32 NewX, int32 NewY);

	/* Auto Placement */
	bool FindEmptySpace(UItemInstance* Item, int32& OutX, int32& OutY);

	/* Item Access */
	UItemInstance* GetItemAtSlot(int32 X, int32 Y) const;
	const TArray<FInventorySlot>& GetSlots() const;
};
