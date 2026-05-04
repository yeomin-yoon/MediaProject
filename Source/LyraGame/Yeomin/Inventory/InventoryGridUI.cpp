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
		if (!Item) continue;

		// 🔥 핵심: Equip된 아이템은 인벤토리에 표시하지 않음
		if (InventoryComp->IsEquipped(Item))
		{
			continue;
		}

		TileViewWidget->AddItem(Item);
	}
}
