#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "InventoryScreenUI.generated.h"

class UInventoryGridUI;
class UBorder;
class ULyraInventoryManagerComponent;
class UInventoryTileUI;
class UDragDropOperation;

UCLASS()
class LYRAGAME_API UInventoryScreenUI : public UCommonActivatableWidget
{
	GENERATED_BODY()
	
protected:
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> EquipSlotBorder1;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> EquipSlotBorder2;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> EquipSlotBorder3;
	
	UPROPERTY(EditDefaultsOnly, Category="UI")
	TSubclassOf<UInventoryTileUI> InventoryTileClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TObjectPtr<USoundBase> EquipSound;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TObjectPtr<USoundBase> UnequipSound;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TObjectPtr<USoundBase> OpenSound;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TObjectPtr<USoundBase> CloseSound;
	
	UPROPERTY()
	TArray<TObjectPtr<UInventoryTileUI>> EquipWidgets;
	
	UPROPERTY()
	TObjectPtr<ULyraInventoryManagerComponent> CachedInventory;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UInventoryGridUI> WidgetGridUI;
	
	virtual void NativeConstruct() override;
	void InitDeferred();

	// 드랍 처리
	virtual bool NativeOnDrop(
		const FGeometry& InGeometry,
		const FDragDropEvent& InDragDropEvent,
		UDragDropOperation* InOperation
	) override;

private:
	void RefreshEquipSlots();
	void RefreshInventoryGrid();
	void UpdateSlot(UBorder* Slot, int32 Index);
};