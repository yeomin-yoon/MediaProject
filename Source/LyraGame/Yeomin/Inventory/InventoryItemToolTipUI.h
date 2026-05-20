// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InventoryFragment_EquipEffect.h"
#include "InventoryItemToolTipUI.generated.h"

class UCommonTextBlock;
class UCommonBorder;
class ULyraInventoryItemInstance;

/**
 * 
 */
UCLASS()
class LYRAGAME_API UInventoryItemToolTipUI : public UUserWidget
{
	GENERATED_BODY()

public:
	void Setup(ULyraInventoryItemInstance* Item);

protected:

	// =========================
	// Root Border
	// =========================

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UCommonBorder> HeaderBorder;

	// =========================
	// Texts
	// =========================

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UCommonTextBlock> RarityText;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UCommonTextBlock> ItemNameText;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UCommonTextBlock> StatText;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UCommonTextBlock> ItemStoryText;
};