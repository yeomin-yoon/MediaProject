#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "InventoryToastUI.generated.h"

class UListView;
class UVerticalBox;
class UItemAcquiredToastEntry;

UCLASS()
class UInventoryToastUI : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;

	void HandleInventoryMessage(FGameplayTag Channel, const struct FLyraInventoryChangeMessage& Message);

	void RemoveToast(UObject* ToastEntry);
	void UpdateDisplayVisibility();

	void ResetToastTimer();
	void HideAllToasts();

protected:
	UPROPERTY(meta = (BindWidget), BlueprintReadOnly)
	TObjectPtr<UVerticalBox> DisplayVBox;

	UPROPERTY(meta = (BindWidget), BlueprintReadOnly)
	TObjectPtr<UListView> ToastListWidget;

private:
	FGameplayMessageListenerHandle ListenerHandle;
	
	FTimerHandle GlobalToastTimer;
};