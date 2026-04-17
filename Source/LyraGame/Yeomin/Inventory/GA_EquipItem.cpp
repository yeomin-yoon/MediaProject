#include "GA_EquipItem.h"

#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameplayTagContainer.h"
#include "Inventory/LyraInventoryItemInstance.h"
#include "Inventory/LyraInventoryManagerComponent.h"

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

	const ULyraInventoryItemInstance* Item =
		Cast<ULyraInventoryItemInstance>(TriggerEventData->OptionalObject);

	if (!Item)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}
	
	APlayerController* PC =
		Cast<APlayerController>(ActorInfo->PlayerController.Get());
	
	ULyraInventoryManagerComponent* Inventory =
		PC->FindComponentByClass<ULyraInventoryManagerComponent>();

	if (!Inventory)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}
	
	Inventory->RemoveItemInstance(
		const_cast<ULyraInventoryItemInstance*>(Item)
	);
	
	FLyraInventoryChangeMessage Message;
	Message.Instance = const_cast<ULyraInventoryItemInstance*>(Item);
	Message.Delta = -1;

	FGameplayTag Tag = FGameplayTag::RequestGameplayTag(
		TEXT("Lyra.Inventory.Message.StackChanged"));

	UGameplayMessageSubsystem::Get(this)
		.BroadcastMessage(Tag, Message);

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}