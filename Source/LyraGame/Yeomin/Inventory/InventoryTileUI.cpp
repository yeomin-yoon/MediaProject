// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryTileUI.h"

#include "InventoryDragDrop.h"

FReply UInventoryTileUI::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
	{
		return FReply::Handled().DetectDrag(TakeWidget(), EKeys::LeftMouseButton);
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UInventoryTileUI::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent,
	UDragDropOperation*& OutOperation)
{
	UInventoryDragDrop* DragOp = NewObject<UInventoryDragDrop>();

	// DragOp->ItemInstance = ItemInstance;
	// DragOp->SourceSlotIndex = SlotIndex;

	// 👇 마우스 따라다니는 UI
	UInventoryTileUI* DragVisual = CreateWidget<UInventoryTileUI>(GetWorld(), GetClass());
	// DragVisual->SetItem(ItemInstance);

	DragOp->DefaultDragVisual = DragVisual;

	OutOperation = DragOp;
}
