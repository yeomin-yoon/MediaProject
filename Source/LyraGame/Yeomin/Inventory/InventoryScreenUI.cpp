#include "InventoryScreenUI.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "InventoryDragDrop.h"
#include "InventoryTileUI.h"
#include "Components/Border.h"
#include "Inventory/LyraInventoryItemInstance.h"
#include "Inventory/LyraInventoryManagerComponent.h"

void UInventoryScreenUI::NativeOnDeactivated()
{
	Super::NativeOnDeactivated();
	
	DeactivateWidget();
}

void UInventoryScreenUI::NativeConstruct()
{
	Super::NativeConstruct();
	
	APlayerController* PC = GetOwningPlayer();
	CachedInventory = PC->FindComponentByClass<ULyraInventoryManagerComponent>();

	// ASC 가져오기
	APawn* Pawn = GetOwningPlayerPawn();
	APlayerState* PS = Pawn->GetPlayerState();
	if (PS)
	{
		IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(PS);
		if (ASCInterface)
		{
			CachedASC = ASCInterface->GetAbilitySystemComponent();
		}
	}
}

bool UInventoryScreenUI::NativeOnDrop(
	const FGeometry& InGeometry,
	const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation)
{
	UInventoryDragDrop* DragOp = Cast<UInventoryDragDrop>(InOperation);
	if (!DragOp) return false;

	UInventoryTileUI* DraggedWidget = DragOp->DraggedWidget;
	if (!DraggedWidget) return false;

	FVector2D ScreenPos = InDragDropEvent.GetScreenSpacePosition();

	UBorder* TargetSlot = GetDropTarget(ScreenPos);
	if (!TargetSlot) return false;

	HandleDropToSlot(TargetSlot, DraggedWidget);

	return true;
}

UBorder* UInventoryScreenUI::GetDropTarget(const FVector2D& ScreenPos) const
{
	if (EquipSlotBorder1 && EquipSlotBorder1->GetCachedGeometry().IsUnderLocation(ScreenPos))
		return EquipSlotBorder1;

	if (EquipSlotBorder2 && EquipSlotBorder2->GetCachedGeometry().IsUnderLocation(ScreenPos))
		return EquipSlotBorder2;

	if (EquipSlotBorder3 && EquipSlotBorder3->GetCachedGeometry().IsUnderLocation(ScreenPos))
		return EquipSlotBorder3;

	return nullptr;
}

void UInventoryScreenUI::HandleDropToSlot(UBorder* TargetSlot, UInventoryTileUI* DraggedWidget)
{
	if (!TargetSlot || !DraggedWidget) return;

	ULyraInventoryItemInstance* DragItem = DraggedWidget->ItemInstance;
	if (!DragItem) return;

	UInventoryTileUI* ExistingWidget = nullptr;

	if (TargetSlot->GetChildrenCount() > 0)
	{
		ExistingWidget = Cast<UInventoryTileUI>(TargetSlot->GetChildAt(0));
	}

	APawn* Pawn = GetOwningPlayerPawn();
	if (!Pawn) return;

	FGameplayEventData EventData;
	EventData.OptionalObject = DragItem;

	// ----------------------------------
	// 1. Swap
	// ----------------------------------
	if (ExistingWidget && ExistingWidget->ItemInstance)
	{
		ULyraInventoryItemInstance* Temp = ExistingWidget->ItemInstance;

		ExistingWidget->SetItemInstance(DragItem);
		DraggedWidget->SetItemInstance(Temp);
	}
	else
	{
		// ----------------------------------
		// 2. Move
		// ----------------------------------
		DraggedWidget->RemoveItem();

		UInventoryTileUI* NewWidget = CreateWidget<UInventoryTileUI>(
			GetWorld(),
			DraggedWidget->GetClass()
		);

		NewWidget->SetItemInstance(DragItem);

		TargetSlot->ClearChildren();
		TargetSlot->AddChild(NewWidget);
		
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
			Pawn,
			FGameplayTag::RequestGameplayTag("Event.EquipItem"),
			EventData
		);
	}
}