#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "InventoryScreenUI.generated.h"

class UGameplayAbility;
class UAbilitySystemComponent;
class UBorder;
class ULyraInventoryManagerComponent;
class UInventoryTileUI;
class UDragDropOperation;

UCLASS()
class LYRAGAME_API UInventoryScreenUI : public UCommonActivatableWidget
{
	GENERATED_BODY()

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> EquipSlotBorder1;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> EquipSlotBorder2;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> EquipSlotBorder3;
	
	UPROPERTY()
	TObjectPtr<ULyraInventoryManagerComponent> CachedInventory;
	
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> CachedASC;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameplayAbility> GA_EquipItem;
	
	virtual void NativeConstruct() override;

	// 드랍 처리
	virtual bool NativeOnDrop(
		const FGeometry& InGeometry,
		const FDragDropEvent& InDragDropEvent,
		UDragDropOperation* InOperation
	) override;

private:
	// 드랍 슬롯 찾기
	UBorder* GetDropTarget(const FVector2D& ScreenPos) const;

	// 드랍 처리
	void HandleDropToSlot(UBorder* TargetSlot, UInventoryTileUI* DraggedWidget);
};