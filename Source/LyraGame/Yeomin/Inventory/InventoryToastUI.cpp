#include "InventoryToastUI.h"
#include "CommonListView.h"
#include "Components/VerticalBox.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "NativeGameplayTags.h"
#include "ItemAcquiredToastEntry.h"
#include "Inventory/LyraInventoryItemDefinition.h"
#include "Inventory/LyraInventoryItemInstance.h"
#include "Inventory/LyraInventoryManagerComponent.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(
	TAG_Lyra_Inventory_Message_StackChanged,
	"Lyra.Inventory.Message.StackChanged"
);

void UInventoryToastUI::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	UGameplayMessageSubsystem& Subsystem =
		UGameplayMessageSubsystem::Get(GetWorld());

	ListenerHandle =
		Subsystem.RegisterListener<FLyraInventoryChangeMessage>(
			TAG_Lyra_Inventory_Message_StackChanged,
			this,
			&UInventoryToastUI::HandleInventoryMessage
		);
}

void UInventoryToastUI::NativeDestruct()
{
	ListenerHandle.Unregister();
	Super::NativeDestruct();
}

void UInventoryToastUI::HandleInventoryMessage(
	FGameplayTag Channel,
	const FLyraInventoryChangeMessage& Message)
{
	if (!Message.Instance)
		return;

	if (Message.Delta <= 0)
		return;
	
	FString DefName = Message.Instance->GetItemDef()->GetName();

	if (!DefName.Contains(TEXT("EquipItem")))
	{
		return;
	}

	UItemAcquiredToastEntry* Entry =
		NewObject<UItemAcquiredToastEntry>(this);

	Entry->ItemInstance = Message.Instance;

	// =========================
	// OptionType (Message 기준)
	// =========================
	switch (Message.OptionType)
	{
	case EItemOptionType::Attack:
		Entry->ItemText = FText::FromString(TEXT("1x Shard of Attack"));
		break;

	case EItemOptionType::Health:
		Entry->ItemText = FText::FromString(TEXT("1x Shard of Vitality"));
		break;

	case EItemOptionType::Stamina:
		Entry->ItemText = FText::FromString(TEXT("1x Shard of Endurance"));
		break;
	}

	Entry->Icon = Message.Instance->GetIconTexture();

	// =========================
	// Rarity
	// =========================
	FLinearColor Color = FLinearColor::White;

	switch (Message.Rarity)
	{
	case EItemRarity::Common:
		Color = FLinearColor(0.65f, 0.65f, 0.65f);
		break;

	case EItemRarity::Uncommon:
		Color = FLinearColor(0.25f, 0.55f, 1.0f);
		break;

	case EItemRarity::Rare:
		Color = FLinearColor(0.7f, 0.35f, 1.0f);
		break;

	case EItemRarity::Epic:
		Color = FLinearColor(1.0f, 0.82f, 0.2f);
		break;
	}

	Entry->RarityColor = Color;

	ToastListWidget->AddItem(Entry);

	UpdateDisplayVisibility();
	ResetToastTimer();
}

void UInventoryToastUI::RemoveToast(UObject* ToastEntry)
{
	if (!ToastListWidget || !ToastEntry)
		return;

	ToastListWidget->RemoveItem(ToastEntry);

	UpdateDisplayVisibility();
}

void UInventoryToastUI::UpdateDisplayVisibility()
{
	if (!DisplayVBox || !ToastListWidget)
		return;

	const bool bVisible = ToastListWidget->GetNumItems() > 0;

	DisplayVBox->SetVisibility(
		bVisible ? ESlateVisibility::HitTestInvisible
				 : ESlateVisibility::Collapsed
	);
}

void UInventoryToastUI::ResetToastTimer()
{
	if (!GetWorld())
		return;

	// 기존 타이머 제거 (핵심)
	GetWorld()->GetTimerManager().ClearTimer(GlobalToastTimer);

	// 새 타이머 시작
	GetWorld()->GetTimerManager().SetTimer(
		GlobalToastTimer,
		this,
		&UInventoryToastUI::HideAllToasts,
		2.0f,
		false
	);
}

void UInventoryToastUI::HideAllToasts()
{
	if (!ToastListWidget)
		return;

	ToastListWidget->ClearListItems();
	UpdateDisplayVisibility();
}