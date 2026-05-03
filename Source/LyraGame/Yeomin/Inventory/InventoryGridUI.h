// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InventoryGridUI.generated.h"

/**
 * 
 */

class ULyraInventoryManagerComponent;
class UCommonTileView;

UCLASS()
class LYRAGAME_API UInventoryGridUI : public UUserWidget
{
	GENERATED_BODY()
	
public:
	virtual void NativeConstruct() override;
	void InitInventory();
	
	UPROPERTY(meta = (BindWidget), BlueprintReadWrite)
	TObjectPtr<UCommonTileView> TileViewWidget;

private:
	UPROPERTY()
	TObjectPtr<ULyraInventoryManagerComponent> InventoryComp = nullptr;
};
