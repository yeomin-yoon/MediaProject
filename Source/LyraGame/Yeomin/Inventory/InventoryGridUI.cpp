// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryGridUI.h"
#include "Inventory/LyraInventoryManagerComponent.h"
#include "CommonTileView.h"
#include "Inventory/LyraInventoryItemInstance.h"

UE_DEFINE_GAMEPLAY_TAG(TAG_Lyra_Inventory_Message_StackChanged, "Lyra.Inventory.Message.StackChanged")

void UInventoryGridUI::NativeConstruct()
{
	Super::NativeConstruct();
	
	UE_LOG(LogTemp, Warning, TEXT("Listener Registered"))

	APlayerController* PC = GetOwningPlayer();
	if (!PC) return;

	InventoryComp = PC->FindComponentByClass<ULyraInventoryManagerComponent>();
	if (!InventoryComp) return;

	// 1. 초기 전체 로딩
	InitInventory();

	// 2. 메시지 구독
	UGameplayMessageSubsystem& Subsystem = UGameplayMessageSubsystem::Get(GetWorld());

	MessageHandle = Subsystem.RegisterListener<FLyraInventoryChangeMessage>(
	TAG_Lyra_Inventory_Message_StackChanged,
	this,
	&UInventoryGridUI::OnInventoryChanged
);
}

void UInventoryGridUI::NativeDestruct()
{
	if (MessageHandle.IsValid())
	{
		UGameplayMessageSubsystem& Subsystem = UGameplayMessageSubsystem::Get(this);
		Subsystem.UnregisterListener(MessageHandle);
	}

	Super::NativeDestruct();
}

void UInventoryGridUI::InitInventory()
{
	if (!TileViewWidget || !InventoryComp) return;

	TileViewWidget->ClearListItems();

	const TArray<ULyraInventoryItemInstance*>& Items = InventoryComp->GetAllItems();

	for (ULyraInventoryItemInstance* Item : Items)
	{
		if (!Item) continue;

		TileViewWidget->AddItem(Item);
	}
}

void UInventoryGridUI::OnInventoryChanged(
	FGameplayTag Channel,
	const FLyraInventoryChangeMessage& Msg)
{
	UE_LOG(LogTemp, Warning, TEXT("Message Received"))
	
	if (!TileViewWidget) return;

	ULyraInventoryItemInstance* Item = Msg.Instance;
	int32 NewCount = Msg.NewCount;

	if (!Item) return;

	// 제거
	if (NewCount == 0)
	{
		TileViewWidget->RemoveItem(Item);
		return;
	}

	// 추가
	const TArray<UObject*>& Items = TileViewWidget->GetListItems();

	if (!Items.Contains(Item))
	{
		TileViewWidget->AddItem(Item);
	}
}