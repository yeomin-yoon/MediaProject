// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "InventoryDragDrop.generated.h"

class UInventoryTileUI;
/**
 * 
 */
UCLASS()
class LYRAGAME_API UInventoryDragDrop : public UDragDropOperation
{
	GENERATED_BODY()
	
public:
	UPROPERTY()
	TObjectPtr<UInventoryTileUI> DraggedWidget;
};
