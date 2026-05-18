// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryGridUI.h"
#include "Inventory/LyraInventoryManagerComponent.h"
#include "CommonTileView.h"
#include "Inventory/LyraInventoryItemInstance.h"

void UInventoryGridUI::NativeConstruct()
{
	Super::NativeConstruct();

	InventoryComp = GetOwningPlayer()
		? GetOwningPlayer()->FindComponentByClass<ULyraInventoryManagerComponent>()
		: nullptr;

	// 한 프레임 뒤로 미룸
	FTimerHandle TimerHandle;
	GetWorld()->GetTimerManager().SetTimer(
		TimerHandle,
		this,
		&UInventoryGridUI::InitInventory,
		0.01f,
		false
	);
}

void UInventoryGridUI::InitInventory()
{
	if (!TileViewWidget || !InventoryComp) return;

	TileViewWidget->ClearListItems();

	const TArray<ULyraInventoryItemInstance*>& Items = InventoryComp->GetAllItems();

	for (ULyraInventoryItemInstance* Item : Items)
	{
		if (!Item)
			continue;

		// Equip 슬롯 제외
		if (InventoryComp->IsEquipped(Item))
		{
			continue;
		}

		FString DefName =
			Item->GetItemDef()->GetName();

		// 랜덤 장비만 표시
		if (!DefName.Contains("EquipItem"))
		{
			continue;
		}

		TileViewWidget->AddItem(Item);
	}
}
