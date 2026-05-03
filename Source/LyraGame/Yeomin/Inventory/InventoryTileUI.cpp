// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryTileUI.h"
#include "InventoryDragDrop.h"
#include "Components/Image.h"
#include "Inventory/InventoryFragment_QuickBarIcon.h"
#include "Inventory/LyraInventoryItemInstance.h"
#include "Inventory/LyraInventoryManagerComponent.h"

void UInventoryTileUI::NativeConstruct()
{
	Super::NativeConstruct();
	
	APlayerController* PC = GetOwningPlayer();
	if (PC)
	{
		CachedInventory = PC->FindComponentByClass<ULyraInventoryManagerComponent>();
	}
}

FReply UInventoryTileUI::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
	{
		return FReply::Handled().DetectDrag(TakeWidget(), EKeys::LeftMouseButton);
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UInventoryTileUI::NativeOnDragDetected(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent,
	UDragDropOperation*& OutOperation)
{
	if (!ItemInstance)
		return;

	UInventoryDragDrop* DragOp = NewObject<UInventoryDragDrop>();

	// 👉 index 삭제, 무조건 pointer
	DragOp->Item = ItemInstance;

	UInventoryTileUI* DragVisual =
		CreateWidget<UInventoryTileUI>(GetWorld(), GetClass());

	DragVisual->SetItemInstance(ItemInstance);

	DragOp->DefaultDragVisual = DragVisual;

	OutOperation = DragOp;
}


void UInventoryTileUI::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	ULyraInventoryItemInstance* Item = Cast<ULyraInventoryItemInstance>(ListItemObject);
	if (!Item) return;

	ItemInstance = Item;

	const UInventoryFragment_QuickBarIcon* IconFragment =
		Item->FindFragmentByClass<UInventoryFragment_QuickBarIcon>();

	if (!IconFragment || !TileIMG) return;

	TileIMG->SetBrush(IconFragment->Brush);
}

void UInventoryTileUI::SetItemInstance(ULyraInventoryItemInstance* NewItem)
{
	ItemInstance = NewItem;
	
	if (!ItemInstance)
	{
		if (TileIMG)
		{
			TileIMG->SetBrushFromTexture(nullptr);
		}
		return;
	}

	const auto* Fragment = ItemInstance->FindFragmentByClass<UInventoryFragment_QuickBarIcon>();
    
	if (Fragment)
	{
		TileIMG->SetBrush(Fragment->Brush);
	}
}

void UInventoryTileUI::RemoveItem()
{
	ItemInstance = nullptr;
	
	if (TileIMG)
	{
		TileIMG->SetBrushFromTexture(nullptr);
	}
}

bool UInventoryTileUI::NativeOnDrop(
	const FGeometry& InGeometry,
	const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation)
{
	return false;
}