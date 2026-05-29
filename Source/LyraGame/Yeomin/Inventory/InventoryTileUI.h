#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "InventoryTileUI.generated.h"

class ALyraWorldCollectable;
class UInventoryItemToolTipUI;
class ULyraInventoryManagerComponent;
class UImage;
class ULyraInventoryItemInstance;

UCLASS()
class LYRAGAME_API UInventoryTileUI : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()
	
public:
	// ListView에서 데이터 들어올 때 호출됨
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;

public:
	UPROPERTY(BlueprintReadWrite, Category="Inventory")
	TObjectPtr<ULyraInventoryItemInstance> ItemInstance = nullptr;
	
	UPROPERTY()
	TObjectPtr<UInventoryItemToolTipUI> CachedTooltip = nullptr;
	
	void SetItemInstance(ULyraInventoryItemInstance* NewItem);
	
	void RemoveItem();

protected:
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<UImage> TileIMG;

	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDown(
		const FGeometry& InGeometry,
		const FPointerEvent& InMouseEvent
	) override;

	virtual void NativeOnDragDetected(
		const FGeometry& InGeometry,
		const FPointerEvent& InMouseEvent,
		UDragDropOperation*& OutOperation
	) override;
	
	virtual bool NativeOnDrop(
		const FGeometry& InGeometry,
		const FDragDropEvent& InDragDropEvent,
		UDragDropOperation* InOperation
	) override;
	
	virtual void NativeOnDragCancelled(
		const FDragDropEvent& InDragDropEvent,
		UDragDropOperation* InOperation
	) override;
	
	UPROPERTY(EditDefaultsOnly, Category="Drop")
	TSubclassOf<ALyraWorldCollectable> ItemDropClass;

	UPROPERTY(EditDefaultsOnly, Category = "Drop")
	TObjectPtr<USoundBase> DropSound;
	
	UPROPERTY(EditDefaultsOnly, Category = "Drop")
	float DropSoundVolume = 1.0f;
	
	UPROPERTY(EditAnywhere)
	TSubclassOf<UInventoryItemToolTipUI> TooltipClass;
};