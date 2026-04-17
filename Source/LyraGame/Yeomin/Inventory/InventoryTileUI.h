// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InventoryTileUI.generated.h"

class UImage;
class ULyraInventoryItemInstance;

UCLASS()
class LYRAGAME_API UInventoryTileUI : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category="Inventory")
	TObjectPtr<ULyraInventoryItemInstance> ItemInstance = nullptr;
	
	void SetItemInstance(ULyraInventoryItemInstance* NewItem);

protected:
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<UImage> TileIMG;
	
	virtual FReply NativeOnMouseButtonDown(
		const FGeometry& InGeometry,
		const FPointerEvent& InMouseEvent
	) override;

	virtual void NativeOnDragDetected(
		const FGeometry& InGeometry,
		const FPointerEvent& InMouseEvent,
		UDragDropOperation*& OutOperation
	) override;
};
