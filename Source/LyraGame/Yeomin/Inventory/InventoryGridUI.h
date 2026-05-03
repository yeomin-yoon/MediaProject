// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "InventoryGridUI.generated.h"

/**
 * 
 */

struct FLyraInventoryChangeMessage;
class ULyraInventoryManagerComponent;
class UCommonTileView;

UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Lyra_Inventory_Message_StackChanged)

UCLASS()
class LYRAGAME_API UInventoryGridUI : public UUserWidget
{
	GENERATED_BODY()
	
public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	void InitInventory();
	void OnInventoryChanged(FGameplayTag Channel, const FLyraInventoryChangeMessage& Msg);
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTileView> TileViewWidget;

	UPROPERTY()
	TObjectPtr<ULyraInventoryManagerComponent> InventoryComp;
	
	FGameplayMessageListenerHandle MessageHandle;
};
