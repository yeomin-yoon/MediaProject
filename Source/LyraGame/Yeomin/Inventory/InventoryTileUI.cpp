// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryTileUI.h"
#include "InventoryDragDrop.h"
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
	// 옵션 타입별 설정
	// =========================

	FString Prefix;
	int32 MaxIndex = 0;

	switch (ItemInstance->OptionType)
	{
	case EItemOptionType::Attack:
		Prefix = TEXT("RedFragment_");
		MaxIndex = 14;
		break;

	case EItemOptionType::Health:
		Prefix = TEXT("YellowFragment_");
		MaxIndex = 13;
		break;

	case EItemOptionType::Stamina:
		Prefix = TEXT("BlueFragment_");
		MaxIndex = 11;
		break;

	default:
		return;
	}

	// =========================
	// Seed 기반 랜덤
	// =========================

	FRandomStream Stream(
		ItemInstance->RandomSeed);

	int32 IconIndex =
		Stream.RandRange(0, MaxIndex);

	FString AssetPath = FString::Printf(
		TEXT("/Game/Loot_Drop_VFX/LootUIIMG/%s%d.%s%d"),
		*Prefix,
		IconIndex,
		*Prefix,
		IconIndex
	);

	UTexture2D* LoadedTexture =
		LoadObject<UTexture2D>(
			nullptr,
			*AssetPath
		);

	if (!LoadedTexture)
	{
		UE_LOG(LogTemp, Error,
			TEXT("Texture Load Failed"));
		return;
	}

	TileIMG->SetBrushFromTexture(
		LoadedTexture);
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