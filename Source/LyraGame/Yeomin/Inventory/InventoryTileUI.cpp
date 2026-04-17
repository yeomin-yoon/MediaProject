// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryTileUI.h"
#include "InventoryDragDrop.h"
#include "Components/Image.h"
#include "Inventory/InventoryFragment_QuickBarIcon.h"
#include "Inventory/LyraInventoryItemInstance.h"

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
	
	DragOp->DraggedWidget = this;
	
	UInventoryTileUI* DragVisual = CreateWidget<UInventoryTileUI>(GetWorld(), GetClass());
	DragVisual->SetItemInstance(this->ItemInstance);
	DragOp->DefaultDragVisual = DragVisual;

	OutOperation = DragOp;
}

void UInventoryTileUI::SetItemInstance(ULyraInventoryItemInstance* NewItem)
{
	ItemInstance = NewItem;

	const auto* Fragment = ItemInstance->FindFragmentByClass<UInventoryFragment_QuickBarIcon>();
	
	TileIMG->SetBrush(Fragment->Brush);
}