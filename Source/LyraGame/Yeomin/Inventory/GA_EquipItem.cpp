#include "GA_EquipItem.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameplayTagContainer.h"
#include "Inventory/LyraInventoryItemInstance.h"
#include "Inventory/LyraInventoryManagerComponent.h"

UGA_EquipItem::UGA_EquipItem()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;

	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = FGameplayTag::RequestGameplayTag("Event.EquipItem");
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;

	AbilityTriggers.Add(TriggerData);
}

void UGA_EquipItem::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!TriggerEventData)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	const ULyraInventoryItemInstance* ItemConst =
	Cast<ULyraInventoryItemInstance>(TriggerEventData->OptionalObject);

	if (!ItemConst)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	ULyraInventoryItemInstance* Item =
		const_cast<ULyraInventoryItemInstance*>(ItemConst);

	if (!Item)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	APlayerController* PC =
		Cast<APlayerController>(ActorInfo->PlayerController.Get());

	if (!PC)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	ULyraInventoryManagerComponent* Inventory =
		PC->FindComponentByClass<ULyraInventoryManagerComponent>();

	if (!Inventory)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	// 인벤토리에서 제거
	Inventory->RemoveItemInstance(Item);

	// UI 갱신용 메시지
	FLyraInventoryChangeMessage Message;
	Message.Instance = Item;
	Message.Delta = -1;

	FGameplayTag Tag = FGameplayTag::RequestGameplayTag(
		TEXT("Lyra.Inventory.Message.StackChanged"));

	UGameplayMessageSubsystem::Get(this)
		.BroadcastMessage(Tag, Message);

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}