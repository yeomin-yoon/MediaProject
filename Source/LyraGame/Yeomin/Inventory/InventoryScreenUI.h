#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "InventoryScreenUI.generated.h"

class ULyraInventoryItemInstance;
class UInventoryGridUI;
class UGameplayAbility;
class UAbilitySystemComponent;
class UBorder;
class ULyraInventoryManagerComponent;
class UInventoryTileUI;
class UDragDropOperation;

USTRUCT()
struct FEquipSlotRef
{
	GENERATED_BODY()

	UPROPERTY()
	int32 SlotIndex = INDEX_NONE;

	UPROPERTY()
	TObjectPtr<UBorder> Border = nullptr;
};

UCLASS()
class LYRAGAME_API UInventoryScreenUI : public UCommonActivatableWidget
{
	GENERATED_BODY()
	
protected:
	virtual void NativeOnDeactivated() override;
	
	UPROPERTY()
	TArray<FEquipSlotRef> EquipSlots;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> EquipSlotBorder1;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> EquipSlotBorder2;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> EquipSlotBorder3;
	
	UPROPERTY(EditDefaultsOnly, Category="UI")
	TSubclassOf<class UInventoryTileUI> InventoryTileClass;
	
	UPROPERTY()
	TArray<TObjectPtr<UInventoryTileUI>> EquipWidgets;
	
	UPROPERTY()
	TObjectPtr<ULyraInventoryManagerComponent> CachedInventory;
	
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> CachedASC;
	
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
	void UpdateSlot(UBorder* Slot, int32 Index);
};