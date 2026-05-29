#include "InventoryScreenUI.h"
#include "CommonUIExtensions.h"
#include "InventoryDragDrop.h"
#include "InventoryGridUI.h"
#include "InventoryTileUI.h"
#include "Components/Border.h"
#include "Inventory/LyraInventoryManagerComponent.h"
#include "Kismet/GameplayStatics.h"

void UInventoryScreenUI::NativeOnActivated()
{
	Super::NativeOnActivated();

	// 여는 소리 재생 (Highlight)
	USoundBase* SoundToPlay = OpenSound.Get();
	if (!SoundToPlay)
	{
		SoundToPlay = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/Sounds/UI/sfx_UI_SubMenu_Highlight_nl_meta_Preset.sfx_UI_SubMenu_Highlight_nl_meta_Preset"));
	}

	if (SoundToPlay)
	{
		UGameplayStatics::PlaySound2D(GetWorld(), SoundToPlay);
	}
}

void UInventoryScreenUI::NativeOnDeactivated()
{
	Super::NativeOnDeactivated();

	if (CachedInventory)
	{
		CachedInventory->OnEquipChanged.RemoveAll(this);
	}

	// 닫는 소리 재생 (Select)
	USoundBase* SoundToPlay = CloseSound.Get();
	if (!SoundToPlay)
	{
		SoundToPlay = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/Sounds/UI/sfx_UI_SubMenu_Select_nl_meta_Preset.sfx_UI_SubMenu_Select_nl_meta_Preset"));
	}

	if (SoundToPlay)
	{
		UGameplayStatics::PlaySound2D(GetWorld(), SoundToPlay);
	}

	UCommonUIExtensions::PopContentFromLayer(this);
}

void UInventoryScreenUI::NativeConstruct()
{
	Super::NativeConstruct();

	// 1프레임 지연
	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UInventoryScreenUI::InitDeferred);
}

void UInventoryScreenUI::InitDeferred()
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC)
		return;

	CachedInventory = PC->FindComponentByClass<ULyraInventoryManagerComponent>();
	if (!IsValid(CachedInventory))
		return;

	// Equip Widgets 생성
	EquipWidgets.SetNum(3);

	for (int32 i = 0; i < 3; i++)
	{
		EquipWidgets[i] = CreateWidget<UInventoryTileUI>(
			GetOwningPlayer(),
			InventoryTileClass
		);
	}

	// Delegate 안전 바인딩
	CachedInventory->OnInventoryChanged.RemoveAll(this);
	CachedInventory->OnInventoryChanged.AddUObject(this, &UInventoryScreenUI::RefreshEquipSlots);
	CachedInventory->OnInventoryChanged.AddUObject(this, &UInventoryScreenUI::RefreshInventoryGrid);
	
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
	
	DragOp->bDroppedOnValidTarget = true;

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

		DragOp->bDroppedOnValidTarget = true;
		
		if (WidgetGridUI)
			WidgetGridUI->InitInventory();

		RefreshEquipSlots();

		// 장착 사운드 재생 (ZoomIn)
		USoundBase* SoundToPlay = EquipSound.Get();
		if (!SoundToPlay)
		{
			SoundToPlay = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/MetaSounds/sfx_ZoomIn_nl_meta_Preset.sfx_ZoomIn_nl_meta_Preset"));
		}

		if (SoundToPlay)
		{
			UGameplayStatics::PlaySound2D(GetWorld(), SoundToPlay);
		}

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

		// 해제 사운드 재생 (ZoomOut)
		USoundBase* SoundToPlay = UnequipSound.Get();
		if (!SoundToPlay)
		{
			SoundToPlay = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/MetaSounds/sfx_ZoomOut_nl_meta_Preset.sfx_ZoomOut_nl_meta_Preset"));
		}

		if (SoundToPlay)
		{
			UGameplayStatics::PlaySound2D(GetWorld(), SoundToPlay);
		}

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

void UInventoryScreenUI::RefreshInventoryGrid()
{
	if (WidgetGridUI)
	{
		WidgetGridUI->InitInventory();
	}
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
