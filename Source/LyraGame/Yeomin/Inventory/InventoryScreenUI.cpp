#include "InventoryScreenUI.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "CommonTileView.h"
#include "CommonUIExtensions.h"
#include "InventoryDragDrop.h"
#include "InventoryGridUI.h"
#include "InventoryTileUI.h"
#include "Components/Border.h"
#include "Inventory/LyraInventoryManagerComponent.h"

void UInventoryScreenUI::NativeOnDeactivated()
{
	Super::NativeOnDeactivated();

	if (CachedInventory)
	{
		CachedInventory->OnEquipChanged.RemoveAll(this);
	}

	UCommonUIExtensions::PopContentFromLayer(this);
}

void UInventoryScreenUI::NativeConstruct()
{
	Super::NativeConstruct();

	// 1프레임 지연 (핵심)
	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UInventoryScreenUI::InitDeferred);
}

void UInventoryScreenUI::InitDeferred()
{
	if (!GetOwningPlayer())
		return;

	APlayerController* PC = GetOwningPlayer();
	if (!PC)
		return;

	CachedInventory = PC->FindComponentByClass<ULyraInventoryManagerComponent>();
	if (!IsValid(CachedInventory))
		return;

	// ASC 가져오기
	APawn* Pawn = GetOwningPlayerPawn();
	APlayerState* PS = Pawn ? Pawn->GetPlayerState() : nullptr;

	if (PS)
	{
		IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(PS);
		if (ASCInterface)
		{
			CachedASC = ASCInterface->GetAbilitySystemComponent();
		}
	}

	// Equip Widgets 생성
	EquipWidgets.SetNum(3);

	for (int32 i = 0; i < 3; i++)
	{
		EquipWidgets[i] = CreateWidget<UInventoryTileUI>(
			GetOwningPlayer(),
			InventoryTileClass
		);
	}

	// Equip Slots 세팅
	EquipSlots.SetNum(3);

	if (EquipSlotBorder1)
	{
		EquipSlots[0].SlotIndex = 0;
		EquipSlots[0].Border = EquipSlotBorder1;
	}

	if (EquipSlotBorder2)
	{
		EquipSlots[1].SlotIndex = 1;
		EquipSlots[1].Border = EquipSlotBorder2;
	}

	if (EquipSlotBorder3)
	{
		EquipSlots[2].SlotIndex = 2;
		EquipSlots[2].Border = EquipSlotBorder3;
	}

	// 🔥 Delegate 안전 바인딩
	if (CachedInventory)
	{
		CachedInventory->OnEquipChanged.RemoveAll(this);
		CachedInventory->OnEquipChanged.AddUObject(this, &UInventoryScreenUI::RefreshEquipSlots);
	}

	RefreshEquipSlots();
}

bool UInventoryScreenUI::NativeOnDrop(
	const FGeometry& InGeometry,
	const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation)
{
	UInventoryDragDrop* DragOp = Cast<UInventoryDragDrop>(InOperation);
	if (!DragOp || !CachedInventory)
		return false;

	const FVector2D ScreenPos = InDragDropEvent.GetScreenSpacePosition();

	int32 SlotIndex = INDEX_NONE;

	if (EquipSlotBorder1 && EquipSlotBorder1->GetCachedGeometry().IsUnderLocation(ScreenPos))
		SlotIndex = 0;
	else if (EquipSlotBorder2 && EquipSlotBorder2->GetCachedGeometry().IsUnderLocation(ScreenPos))
		SlotIndex = 1;
	else if (EquipSlotBorder3 && EquipSlotBorder3->GetCachedGeometry().IsUnderLocation(ScreenPos))
		SlotIndex = 2;

	// =========================
	// 1. EQUIP SLOT
	// =========================
	if (SlotIndex != INDEX_NONE)
	{
		CachedInventory->EquipFromInventory(SlotIndex, DragOp->Item);

		if (WidgetGridUI)
			WidgetGridUI->InitInventory();

		RefreshEquipSlots();
		return true;
	}

	// =========================
	// 2. UN-EQUIP
	// =========================
	if (CachedInventory->IsEquipped(DragOp->Item))
	{
		CachedInventory->RemoveFromEquipAndReturnToInventory(DragOp->Item);

		if (WidgetGridUI)
			WidgetGridUI->InitInventory();

		RefreshEquipSlots();
		return true;
	}

	return false;
}

void UInventoryScreenUI::RefreshEquipSlots()
{
	if (!CachedInventory)
		return;

	UpdateSlot(EquipSlotBorder1, 0);
	UpdateSlot(EquipSlotBorder2, 1);
	UpdateSlot(EquipSlotBorder3, 2);
}

void UInventoryScreenUI::UpdateSlot(UBorder* EquipSlot, int32 Index)
{
	if (!EquipSlot)
		return;

	EquipSlot->ClearChildren();

	if (!CachedInventory || !CachedInventory->EquipSlots.IsValidIndex(Index))
		return;

	UInventoryTileUI* Widget = EquipWidgets.IsValidIndex(Index) ? EquipWidgets[Index] : nullptr;

	if (!Widget)
	{
		Widget = CreateWidget<UInventoryTileUI>(GetOwningPlayer(), InventoryTileClass);
		EquipWidgets[Index] = Widget;
	}

	ULyraInventoryItemInstance* Item = CachedInventory->EquipSlots[Index];

	Widget->SetItemInstance(Item);

	if (!Widget->IsInViewport())
	{
		EquipSlot->AddChild(Widget);
	}
}