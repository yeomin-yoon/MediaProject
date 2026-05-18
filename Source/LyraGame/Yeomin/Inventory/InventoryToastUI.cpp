#include "InventoryToastUI.h"
#include "CommonListView.h"
#include "Components/VerticalBox.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "NativeGameplayTags.h"
#include "ItemAcquiredToastEntry.h"
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

	// =========================
	// EquipItem만 허용
	// =========================

	FString DefName =
		Message.Instance->GetItemDef()->GetName();

	if (!DefName.Contains(TEXT("EquipItem")))
	{
		return;
	}

	// =========================
	// Toast 생성
	// =========================

	UItemAcquiredToastEntry* Entry =
		NewObject<UItemAcquiredToastEntry>(this);

	Entry->ItemInstance = Message.Instance;

	FString ItemName;

	switch (Message.Instance->OptionType)
	{
	case EItemOptionType::Attack:
		ItemName = TEXT("Shard of Attack");
		break;

	case EItemOptionType::Health:
		ItemName = TEXT("Shard of Vitality");
		break;

	case EItemOptionType::Stamina:
		ItemName = TEXT("Shard of Endurance");
		break;
	}

	Entry->ItemText = FText::FromString(
		FString::Printf(TEXT("1x %s"), *ItemName)
	);
	
	// =========================
	// 아이콘 생성
	// =========================

	FString Prefix;
	int32 MaxIndex = 0;

	switch (Message.Instance->OptionType)
	{
	case EItemOptionType::Attack:
		Prefix = TEXT("RedFragment_");
		MaxIndex = 14;
		break;

	case EItemOptionType::Health:
		Prefix = TEXT("YellowFragment_");
		MaxIndex = 13;
		break;

	case EItemOptionType::Stamina:
		Prefix = TEXT("BlueFragment_");
		MaxIndex = 11;
		break;
	}

	FRandomStream Stream(
		Message.Instance->RandomSeed);

	int32 IconIndex =
		Stream.RandRange(0, MaxIndex);

	FString AssetPath = FString::Printf(
		TEXT("/Game/Loot_Drop_VFX/LootUIIMG/%s%d.%s%d"),
		*Prefix,
		IconIndex,
		*Prefix,
		IconIndex
	);

	Entry->Icon =
		LoadObject<UTexture2D>(
			nullptr,
			*AssetPath
		);

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