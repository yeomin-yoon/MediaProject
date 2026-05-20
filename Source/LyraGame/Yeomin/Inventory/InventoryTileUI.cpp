// Fill out your copyright notice in the Description page of Project Settings.


// Fill out your copyright notice in the Description page of Project Settings.

#include "InventoryTileUI.h"
#include "InventoryDragDrop.h"
#include "InventoryItemToolTipUI.h"
#include "LyraWorldCollectable.h"
#include "Components/Image.h"
#include "Inventory/LyraInventoryItemInstance.h"
#include "Inventory/LyraInventoryManagerComponent.h"

void UInventoryTileUI::NativeOnMouseEnter(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

	if (CachedTooltip)
	{
		CachedTooltip->SetVisibility(ESlateVisibility::Visible);
	}
}

void UInventoryTileUI::NativeOnMouseLeave(
	const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);

	if (CachedTooltip)
	{
		CachedTooltip->SetVisibility(ESlateVisibility::Collapsed);
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

void UInventoryTileUI::SetItemInstance(ULyraInventoryItemInstance* NewItem)
{
	ItemInstance = NewItem;

	if (!TileIMG)
		return;

	if (!ItemInstance)
	{
		TileIMG->SetBrushFromTexture(nullptr);
		return;
	}

	// =========================
	// 아이콘
	// =========================
	if (UTexture2D* LoadedTexture = ItemInstance->GetIconTexture())
	{
		TileIMG->SetBrushFromTexture(LoadedTexture);
	}

	// =========================
	// 툴팁 (1회 생성 + 필요 시만 갱신)
	// =========================
	if (TooltipClass)
	{
		if (!CachedTooltip)
		{
			CachedTooltip = CreateWidget<UInventoryItemToolTipUI>(GetWorld(), TooltipClass);
			SetToolTip(CachedTooltip);
			CachedTooltip->SetVisibility(ESlateVisibility::Collapsed);
		}

		// 아이템이 바뀔 때만 Setup
		CachedTooltip->Setup(ItemInstance);
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

void UInventoryTileUI::NativeOnDragCancelled(
	const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation)
{
	Super::NativeOnDragCancelled(InDragDropEvent, InOperation);

	UInventoryDragDrop* DragOp = Cast<UInventoryDragDrop>(InOperation);
	if (!DragOp || DragOp->bDroppedOnValidTarget)
		return;

	if (!ItemInstance)
		return;

	APlayerController* PC = GetOwningPlayer();
	if (!PC)
		return;

	ULyraInventoryManagerComponent* Inventory =
		PC->FindComponentByClass<ULyraInventoryManagerComponent>();

	if (!Inventory)
		return;

	APawn* Pawn = PC->GetPawn();
	if (!Pawn)
		return;

	UWorld* World = GetWorld();
	if (!World)
		return;

	// =========================
	// 1. 방향 계산 (3인칭 기준)
	// =========================
	FVector Forward = Pawn->GetActorForwardVector();
	FVector Right = Pawn->GetActorRightVector();

	// =========================
	// 2. Spawn 위치 (캐릭터 앞 + 오른쪽 랜덤)
	// =========================
	FVector SpawnLocation =
		Pawn->GetActorLocation()
		+ Forward * 80.f
		+ Right * FMath::RandRange(-30.f, 30.f)
		+ FVector(0, 0, 50.f);

	FActorSpawnParameters Params;
	Params.Owner = Pawn;
	Params.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	// =========================
	// 3. 월드 아이템 생성
	// =========================
	ALyraWorldCollectable* DroppedItem =
		World->SpawnActor<ALyraWorldCollectable>(
			ItemDropClass,
			SpawnLocation,
			FRotator::ZeroRotator,
			Params
		);

	if (!DroppedItem)
		return;

	// =========================
	// 4. 데이터 복사
	// =========================
	FInventoryPickup Pickup;

	FPickupTemplate Template;
	Template.ItemDef = ItemInstance->GetItemDef();
	Template.StackCount = 1;
	Template.RandomSeed = ItemInstance->RandomSeed;
	Template.OptionType = ItemInstance->OptionType;
	Template.Rarity = ItemInstance->Rarity;

	Pickup.Templates.Add(Template);

	DroppedItem->StaticInventory = Pickup;

	// =========================
	// 5. FX 세팅
	// =========================
	DroppedItem->OptionType = ItemInstance->OptionType;
	DroppedItem->Rarity = ItemInstance->Rarity;
	DroppedItem->RandomSeed = ItemInstance->RandomSeed;

	DroppedItem->ApplyNiagaraByOption();

	// =========================
	// 6. 튀어나오는 힘
	// =========================
	FVector LaunchDir =
		Right * FMath::RandRange(40.f, 41.f)
		+ Forward * FMath::RandRange(150.f, 220.f)
		+ FVector(0, 0, FMath::RandRange(515.f, 520.f));
	
	DroppedItem->LaunchItem(LaunchDir);

	// =========================
	// 7. 인벤토리 + 장착 제거
	// =========================
	Inventory->RemoveItemFromAnywhere(ItemInstance);

	// =========================
	// 8. UI 제거
	// =========================
	RemoveItem();
}