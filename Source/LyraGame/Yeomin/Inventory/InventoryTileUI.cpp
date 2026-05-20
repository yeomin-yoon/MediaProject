// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryTileUI.h"
#include "InventoryDragDrop.h"
#include "InventoryItemToolTipUI.h"
#include "Components/Image.h"
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
	
	DragOp->Item = ItemInstance;

	UInventoryTileUI* DragVisual =
		CreateWidget<UInventoryTileUI>(GetWorld(), GetClass());

	DragVisual->SetItemInstance(ItemInstance);

	DragOp->DefaultDragVisual = DragVisual;

	OutOperation = DragOp;
}


void UInventoryTileUI::NativeOnListItemObjectSet(
	UObject* ListItemObject)
{
	ULyraInventoryItemInstance* Item =
		Cast<ULyraInventoryItemInstance>(
			ListItemObject);

	if (!Item)
		return;

	SetItemInstance(Item);
}

void UInventoryTileUI::SetItemInstance(
	ULyraInventoryItemInstance* NewItem)
{
	ItemInstance = NewItem;

	// =========================
	// 안정성 체크
	// =========================

	if (!TileIMG)
	{
		UE_LOG(LogTemp, Error,
			TEXT("TileIMG Is Null"));
		return;
	}

	if (!ItemInstance)
	{
		TileIMG->SetBrushFromTexture(nullptr);
		return;
	}

	// =========================
	// 아이콘 설정
	// =========================

	UTexture2D* LoadedTexture =
		ItemInstance->GetIconTexture();

	if (LoadedTexture)
	{
		TileIMG->SetBrushFromTexture(
			LoadedTexture);
	}

	// =========================
	// Tooltip 생성
	// =========================

	if (TooltipClass)
	{
		UInventoryItemToolTipUI* Tooltip =
			CreateWidget<UInventoryItemToolTipUI>(
				GetWorld(),
				TooltipClass);

		if (Tooltip)
		{
			Tooltip->Setup(ItemInstance);

			SetToolTip(Tooltip);
		}
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