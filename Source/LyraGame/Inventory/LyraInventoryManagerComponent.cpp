// Copyright Epic Games, Inc. All Rights Reserved.

#include "LyraInventoryManagerComponent.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffectTypes.h"
#include "Engine/ActorChannel.h"
#include "Engine/World.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "LyraInventoryItemDefinition.h"
#include "LyraInventoryItemInstance.h"
#include "NativeGameplayTags.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Player/LyraPlayerController.h"
#include "Yeomin/Inventory/InventoryFragment_EquipEffect.h"
#include "Yeomin/Inventory/InventoryLogChannels.h"
#include "Yeomin/Inventory/InventorySaveSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraInventoryManagerComponent)

class FLifetimeProperty;
struct FReplicationFlags;

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Lyra_Inventory_Message_StackChanged, "Lyra.Inventory.Message.StackChanged");

//////////////////////////////////////////////////////////////////////
// FLyraInventoryEntry

FString FLyraInventoryEntry::GetDebugString() const
{
	TSubclassOf<ULyraInventoryItemDefinition> ItemDef;
	if (Instance != nullptr)
	{
		ItemDef = Instance->GetItemDef();
	}

	return FString::Printf(TEXT("%s (%d x %s)"), *GetNameSafe(Instance), StackCount, *GetNameSafe(ItemDef));
}

//////////////////////////////////////////////////////////////////////
// FLyraInventoryList

void FLyraInventoryList::PreReplicatedRemove(
	const TArrayView<int32> RemovedIndices,
	int32 FinalSize)
{
	for (int32 Index : RemovedIndices)
	{
		FLyraInventoryEntry& Entry = Entries[Index];

		BroadcastChangeMessage(Entry, Entry.StackCount, 0);
		Entry.LastObservedCount = 0;
	}
}

void FLyraInventoryList::PostReplicatedAdd(
	const TArrayView<int32> AddedIndices,
	int32 FinalSize)
{
	for (int32 Index : AddedIndices)
	{
		FLyraInventoryEntry& Entry = Entries[Index];

		BroadcastChangeMessage(Entry, 0, Entry.StackCount);
		Entry.LastObservedCount = Entry.StackCount;
	}
}

void FLyraInventoryList::PostReplicatedChange(
	const TArrayView<int32> ChangedIndices,
	int32 FinalSize)
{
	for (int32 Index : ChangedIndices)
	{
		FLyraInventoryEntry& Entry = Entries[Index];

		BroadcastChangeMessage(
			Entry,
			Entry.LastObservedCount,
			Entry.StackCount
		);

		Entry.LastObservedCount = Entry.StackCount;
	}
}

void FLyraInventoryList::BroadcastChangeMessage(
	FLyraInventoryEntry& Entry,
	int32 OldCount,
	int32 NewCount)
{
	if (!OwnerComponent)
		return;

	UWorld* World = OwnerComponent->GetWorld();
	if (!World)
		return;

	FLyraInventoryChangeMessage Message;
	Message.InventoryOwner = OwnerComponent;
	Message.Instance = Entry.Instance;
	Message.NewCount = NewCount;
	Message.Delta = NewCount - OldCount;

	// =========================
	// Rarity / OptionType 반드시 전달
	// =========================
	if (Entry.Instance)
	{
		Message.Rarity = Entry.Instance->Rarity;
		Message.OptionType = Entry.Instance->OptionType;
	}

	UGameplayMessageSubsystem::Get(World)
		.BroadcastMessage(
			TAG_Lyra_Inventory_Message_StackChanged,
			Message
		);
}

ULyraInventoryItemInstance* FLyraInventoryList::AddEntry(
	TSubclassOf<ULyraInventoryItemDefinition> ItemDef,
	int32 StackCount,
	int32 RandomSeed,
	EItemOptionType OptionType,
	EItemRarity Rarity)
{
	check(ItemDef);
	check(OwnerComponent);

	AActor* Owner = OwnerComponent->GetOwner();
	check(Owner->HasAuthority());

	FLyraInventoryEntry& NewEntry = Entries.AddDefaulted_GetRef();

	NewEntry.Instance = NewObject<ULyraInventoryItemInstance>(Owner);
	NewEntry.Instance->SetItemDef(ItemDef);

	// Drop에서 만든 값 그대로 주입
	NewEntry.Instance->RandomSeed = RandomSeed;
	NewEntry.Instance->OptionType = OptionType;
	NewEntry.Instance->Rarity = Rarity;

	NewEntry.StackCount = StackCount;

	MarkItemDirty(NewEntry);

	return NewEntry.Instance;
}

void FLyraInventoryList::AddEntry(ULyraInventoryItemInstance* Instance)
{
	if (!Instance || !OwnerComponent)
		return;

	AActor* Owner = OwnerComponent->GetOwner();
	if (!Owner || !Owner->HasAuthority())
		return;

	FLyraInventoryEntry& NewEntry = Entries.AddDefaulted_GetRef();

	NewEntry.Instance = Instance;
	NewEntry.StackCount = 1;
	NewEntry.LastObservedCount = 1;
	
	MarkItemDirty(NewEntry);

	BroadcastChangeMessage(NewEntry, 0, 1);
}

void FLyraInventoryList::RemoveEntry(ULyraInventoryItemInstance* Instance)
{
	for (int32 i = 0; i < Entries.Num(); i++)
	{
		if (Entries[i].Instance == Instance)
		{
			BroadcastChangeMessage(Entries[i], Entries[i].StackCount, 0);

			Entries.RemoveAt(i);
			MarkArrayDirty();
			break;
		}
	}
}

TArray<ULyraInventoryItemInstance*> FLyraInventoryList::GetAllItems() const
{
	TArray<ULyraInventoryItemInstance*> Results;
	Results.Reserve(Entries.Num());
	for (const FLyraInventoryEntry& Entry : Entries)
	{
		if (Entry.Instance != nullptr) //@TODO: Would prefer to not deal with this here and hide it further?
		{
			Results.Add(Entry.Instance);
		}
	}
	return Results;
}

void FLyraInventoryList::SwapEntries(
	ULyraInventoryItemInstance* A,
	ULyraInventoryItemInstance* B)
{
	if (!A || !B) return;

	int32 IndexA = INDEX_NONE;
	int32 IndexB = INDEX_NONE;

	for (int32 i = 0; i < Entries.Num(); i++)
	{
		if (Entries[i].Instance == A)
		{
			IndexA = i;
		}
		else if (Entries[i].Instance == B)
		{
			IndexB = i;
		}
	}

	if (IndexA == INDEX_NONE || IndexB == INDEX_NONE)
		return;

	// 핵심: FastArray 안전 Swap
	Entries.Swap(IndexA, IndexB);

	MarkItemDirty(Entries[IndexA]);
	MarkItemDirty(Entries[IndexB]);
}

//////////////////////////////////////////////////////////////////////
// ULyraInventoryManagerComponent

ULyraInventoryManagerComponent::ULyraInventoryManagerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, InventoryList(this)
{
	SetIsReplicatedByDefault(true);
	
	EquipSlots.SetNum(3);
}

void ULyraInventoryManagerComponent::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, InventoryList);
}

bool ULyraInventoryManagerComponent::CanAddItemDefinition(TSubclassOf<ULyraInventoryItemDefinition> ItemDef, int32 StackCount)
{
	//@TODO: Add support for stack limit / uniqueness checks / etc...
	return ItemDef != nullptr && StackCount > 0;
}

ULyraInventoryItemInstance* ULyraInventoryManagerComponent::AddItemDefinition(
	TSubclassOf<ULyraInventoryItemDefinition> ItemDef,
	int32 StackCount)
{
	return AddItemDefinition(
		ItemDef,
		StackCount,
		0,
		EItemOptionType::Attack,
		EItemRarity::Common
	);
}
ULyraInventoryItemInstance* ULyraInventoryManagerComponent::AddItemDefinition(
	TSubclassOf<ULyraInventoryItemDefinition> ItemDef,
	int32 StackCount,
	int32 RandomSeed,
	EItemOptionType OptionType,
	EItemRarity Rarity)
{
	if (!ItemDef)
		return nullptr;
	
	ULyraInventoryItemInstance* Result =
		InventoryList.AddEntry(ItemDef, StackCount, RandomSeed, OptionType, Rarity);

	if (!Result)
		return nullptr;

	BroadcastItemAdded(Result, StackCount);

	return Result;
}

void ULyraInventoryManagerComponent::AddItemInstance(ULyraInventoryItemInstance* ItemInstance)
{
	if (!ItemInstance)
		return;

	InventoryList.AddEntry(ItemInstance);

	if (IsUsingRegisteredSubObjectList() && IsReadyForReplication())
	{
		AddReplicatedSubObject(ItemInstance);
	}
}

void ULyraInventoryManagerComponent::RemoveItemInstance(ULyraInventoryItemInstance* ItemInstance)
{
	InventoryList.RemoveEntry(ItemInstance);

	if (ItemInstance && IsUsingRegisteredSubObjectList())
	{
		RemoveReplicatedSubObject(ItemInstance);
	}

	OnInventoryChanged.Broadcast();
}

void ULyraInventoryManagerComponent::RemoveItemFromAnywhere(
	ULyraInventoryItemInstance* Item)
{
	if (!Item)
	{
		return;
	}

	// =========================
	// 1. Equip 슬롯 제거
	// =========================

	for (int32 i = 0; i < EquipSlots.Num(); i++)
	{
		if (EquipSlots[i] == Item)
		{
			APlayerController* PC =
				Cast<APlayerController>(GetOwner());

			APlayerState* PS =
				PC ? PC->GetPlayerState<APlayerState>() : nullptr;

			UAbilitySystemComponent* ASC =
				PS ? PS->FindComponentByClass<UAbilitySystemComponent>() : nullptr;

			if (ASC)
			{
				RemoveEquipEffect(ASC, Item);
			}

			EquipSlots[i] = nullptr;
		}
	}

	// =========================
	// 2. Inventory 제거
	// =========================

	InventoryList.RemoveEntry(Item);

	if (IsUsingRegisteredSubObjectList())
	{
		RemoveReplicatedSubObject(Item);
	}

	// =========================
	// 3. UI 이벤트
	// =========================

	OnEquipChanged.Broadcast();
	OnInventoryChanged.Broadcast();

	// =========================
	// 4. Save
	// =========================
	
	SaveInventory();
}

void ULyraInventoryManagerComponent::EquipSwap(int32 SlotIndex, ULyraInventoryItemInstance* NewItem)
{
	if (!NewItem || !EquipSlots.IsValidIndex(SlotIndex))
		return;

	if (EquipSlots[SlotIndex] == NewItem)
		return;

	// 기존 위치 찾기
	int32 OldIndex = INDEX_NONE;

	for (int32 i = 0; i < EquipSlots.Num(); i++)
	{
		if (EquipSlots[i] == NewItem)
		{
			OldIndex = i;
			break;
		}
	}

	// 1. swap 핵심
	ULyraInventoryItemInstance* OldItem = EquipSlots[SlotIndex];

	EquipSlots[SlotIndex] = NewItem;

	if (OldIndex != INDEX_NONE && OldIndex != SlotIndex)
	{
		EquipSlots[OldIndex] = OldItem;
	}

	// 2. UI 동기화 강제 트리거
	OnEquipChanged.Broadcast();
}


TArray<ULyraInventoryItemInstance*> ULyraInventoryManagerComponent::GetAllItems() const
{
	return InventoryList.GetAllItems();
}

ULyraInventoryItemInstance* ULyraInventoryManagerComponent::FindFirstItemStackByDefinition(TSubclassOf<ULyraInventoryItemDefinition> ItemDef) const
{
	for (const FLyraInventoryEntry& Entry : InventoryList.Entries)
	{
		ULyraInventoryItemInstance* Instance = Entry.Instance;

		if (IsValid(Instance))
		{
			if (Instance->GetItemDef() == ItemDef)
			{
				return Instance;
			}
		}
	}

	return nullptr;
}

int32 ULyraInventoryManagerComponent::GetTotalItemCountByDefinition(TSubclassOf<ULyraInventoryItemDefinition> ItemDef) const
{
	int32 TotalCount = 0;
	for (const FLyraInventoryEntry& Entry : InventoryList.Entries)
	{
		ULyraInventoryItemInstance* Instance = Entry.Instance;

		if (IsValid(Instance))
		{
			if (Instance->GetItemDef() == ItemDef)
			{
				++TotalCount;
			}
		}
	}

	return TotalCount;
}

bool ULyraInventoryManagerComponent::ConsumeItemsByDefinition(TSubclassOf<ULyraInventoryItemDefinition> ItemDef, int32 NumToConsume)
{
	AActor* OwningActor = GetOwner();
	if (!OwningActor || !OwningActor->HasAuthority())
	{
		return false;
	}

	//@TODO: N squared right now as there's no acceleration structure
	int32 TotalConsumed = 0;
	while (TotalConsumed < NumToConsume)
	{
		if (ULyraInventoryItemInstance* Instance = ULyraInventoryManagerComponent::FindFirstItemStackByDefinition(ItemDef))
		{
			InventoryList.RemoveEntry(Instance);
			++TotalConsumed;
		}
		else
		{
			return false;
		}
	}

	return TotalConsumed == NumToConsume;
}

void ULyraInventoryManagerComponent::ReadyForReplication()
{
	Super::ReadyForReplication();

	// Register existing ULyraInventoryItemInstance
	if (IsUsingRegisteredSubObjectList())
	{
		for (const FLyraInventoryEntry& Entry : InventoryList.Entries)
		{
			ULyraInventoryItemInstance* Instance = Entry.Instance;

			if (IsValid(Instance))
			{
				AddReplicatedSubObject(Instance);
			}
		}
	}
}

bool ULyraInventoryManagerComponent::ReplicateSubobjects(UActorChannel* Channel, class FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	bool WroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);

	for (FLyraInventoryEntry& Entry : InventoryList.Entries)
	{
		ULyraInventoryItemInstance* Instance = Entry.Instance;

		if (Instance && IsValid(Instance))
		{
			WroteSomething |= Channel->ReplicateSubobject(Instance, *Bunch, *RepFlags);
		}
	}

	return WroteSomething;
}

bool ULyraInventoryManagerComponent::IsEquipped(ULyraInventoryItemInstance* Item) const
{
	if (!Item || !IsValid(this))
		return false;

	for (ULyraInventoryItemInstance* Equipped : EquipSlots)
	{
		if (!IsValid(Equipped))
			continue;

		if (Equipped == Item)
			return true;
	}

	return false;
}

void ULyraInventoryManagerComponent::EquipFromInventory(
	int32 SlotIndex,
	ULyraInventoryItemInstance* Item)
{
	if (!Item || !EquipSlots.IsValidIndex(SlotIndex))
		return;

	// =========================
	// 1. 같은 아이템 다른 슬롯 제거
	// =========================
	for (int32 i = 0; i < EquipSlots.Num(); i++)
	{
		if (EquipSlots[i] == Item)
		{
			EquipSlots[i] = nullptr;
		}
	}

	// =========================
	// 2. 기존 아이템 저장
	// =========================
	ULyraInventoryItemInstance* OldItem = EquipSlots[SlotIndex];

	// =========================
	// 3. 슬롯 교체 (핵심)
	// =========================
	EquipSlots[SlotIndex] = Item;

	// =========================
	// 4. GE 처리 (기존 제거 → 신규 적용)
	// =========================
	APlayerController* PC = Cast<APlayerController>(GetOwner());
	APlayerState* PS = PC ? PC->GetPlayerState<APlayerState>() : nullptr;
	UAbilitySystemComponent* ASC = PS ? PS->FindComponentByClass<UAbilitySystemComponent>() : nullptr;

	if (ASC)
	{
		// 기존 GE 제거
		if (OldItem)
		{
			RemoveEquipEffect(ASC, OldItem);
		}

		// 새 GE 적용
		ApplyEquipEffect(SlotIndex, Item);
	}

	// =========================
	// 5. UI 갱신 트리거
	// =========================
	OnEquipChanged.Broadcast();
	
	// =========================
	// 4. Save
	// =========================
	
	SaveInventory();
}

void ULyraInventoryManagerComponent::SwapEquipSlots(int32 A, int32 B)
{
	if (!EquipSlots.IsValidIndex(A) || !EquipSlots.IsValidIndex(B))
		return;

	if (A == B)
		return;

	APlayerController* PC = Cast<APlayerController>(GetOwner());
	APlayerState* PS = PC ? PC->GetPlayerState<APlayerState>() : nullptr;
	UAbilitySystemComponent* ASC = PS ? PS->FindComponentByClass<UAbilitySystemComponent>() : nullptr;

	ULyraInventoryItemInstance* ItemA = EquipSlots[A];
	ULyraInventoryItemInstance* ItemB = EquipSlots[B];

	EquipSlots[A] = ItemB;
	EquipSlots[B] = ItemA;

	// GE 재적용
	if (ASC)
	{
		if (ItemA)
			ApplyEquipEffect(B, ItemA);

		if (ItemB)
			ApplyEquipEffect(A, ItemB);
	}

	OnEquipChanged.Broadcast();
}

void ULyraInventoryManagerComponent::RemoveFromEquipAndReturnToInventory(
	ULyraInventoryItemInstance* Item)
{
	if (!Item)
		return;

	APlayerController* PC = Cast<APlayerController>(GetOwner());
	APlayerState* PS = PC ? PC->GetPlayerState<APlayerState>() : nullptr;
	UAbilitySystemComponent* ASC = PS ? PS->FindComponentByClass<UAbilitySystemComponent>() : nullptr;

	// =========================
	// 1. 슬롯 제거
	// =========================
	for (int32 i = 0; i < EquipSlots.Num(); i++)
	{
		if (EquipSlots[i] == Item)
		{
			if (ASC)
			{
				RemoveEquipEffect(ASC, Item);
			}

			EquipSlots[i] = nullptr;
			break;
		}
	}

	OnEquipChanged.Broadcast();
	
	// =========================
	// 4. Save
	// =========================
	
	SaveInventory();
}

void ULyraInventoryManagerComponent::ApplyEquipEffect(
	int32 SlotIndex,
	ULyraInventoryItemInstance* Item)
{
	if (!Item)
		return;

	APlayerController* PC =
		Cast<APlayerController>(GetOwner());

	APlayerState* PS =
		PC ? PC->GetPlayerState<APlayerState>() : nullptr;

	UAbilitySystemComponent* ASC =
		PS ? PS->FindComponentByClass<UAbilitySystemComponent>() : nullptr;

	if (!ASC)
		return;

	TSubclassOf<ULyraInventoryItemDefinition> ItemDef = Item->GetItemDef();
	if (!ItemDef)
		return;

	const ULyraInventoryItemDefinition* DefCDO =
		GetDefault<ULyraInventoryItemDefinition>(ItemDef);

	if (!DefCDO)
		return;

	const UInventoryFragment_EquipEffect* Frag =
		Cast<UInventoryFragment_EquipEffect>(
			DefCDO->FindFragmentByClass(
				UInventoryFragment_EquipEffect::StaticClass())
		);

	if (!Frag || !Frag->EquipEffect)
		return;

	// 기존 제거
	RemoveEquipEffect(ASC, Item);

	float AttackValue = 0.f;
	float HealthValue = 0.f;
	float StaminaValue = 0.f;

	float RandomValue =
		Frag->RollRandomValue(Item);

	switch (Item->OptionType)
	{
	case EItemOptionType::Attack:
		AttackValue = RandomValue;
		break;

	case EItemOptionType::Health:
		HealthValue = RandomValue;
		break;

	case EItemOptionType::Stamina:
		StaminaValue = RandomValue;
		break;
	}

	FGameplayEffectContextHandle Context =
		ASC->MakeEffectContext();

	FGameplayEffectSpecHandle Spec =
		ASC->MakeOutgoingSpec(
			Frag->EquipEffect,
			1.f,
			Context);

	if (!Spec.IsValid())
		return;

	// =========================
	// 전부 세팅
	// =========================

	Spec.Data->SetSetByCallerMagnitude(
		FGameplayTag::RequestGameplayTag(
			"SetByCaller.Data.AttackPower"),
		AttackValue
	);

	Spec.Data->SetSetByCallerMagnitude(
		FGameplayTag::RequestGameplayTag(
			"SetByCaller.Data.Health"),
		HealthValue
	);

	Spec.Data->SetSetByCallerMagnitude(
		FGameplayTag::RequestGameplayTag(
			"SetByCaller.Data.Stamina"),
		StaminaValue
	);

	FActiveGameplayEffectHandle Handle =
		ASC->ApplyGameplayEffectSpecToSelf(
			*Spec.Data.Get());

	ActiveGEMap.Add(Item, Handle);
}

void ULyraInventoryManagerComponent::RemoveEquipEffect(
	UAbilitySystemComponent* ASC,
	ULyraInventoryItemInstance* Item)
{
	if (!ASC || !Item)
		return;

	FActiveGameplayEffectHandle* Handle = ActiveGEMap.Find(Item);

	if (Handle && Handle->IsValid())
	{
		ASC->RemoveActiveGameplayEffect(*Handle);
	}

	ActiveGEMap.Remove(Item);
}

FInventorySaveData ULyraInventoryManagerComponent::MakeSaveData() const
{
	FInventorySaveData SaveData;

	for (const FLyraInventoryEntry& Entry : InventoryList.Entries)
	{
		if (!Entry.Instance) continue;

		FInventoryEntrySave Data;
		Data.ItemDef = Entry.Instance->GetItemDef();
		Data.StackCount = Entry.StackCount;

		Data.RandomSeed = Entry.Instance->RandomSeed;
		Data.OptionType = Entry.Instance->OptionType;
		Data.Rarity = Entry.Instance->Rarity;

		// Equip 슬롯
		Data.EquipSlotIndex = INDEX_NONE;
		for (int32 i = 0; i < EquipSlots.Num(); ++i)
		{
			if (EquipSlots[i] == Entry.Instance)
			{
				Data.EquipSlotIndex = i;
				break;
			}
		}

		SaveData.Items.Add(Data);
	}

	return SaveData;
}

void ULyraInventoryManagerComponent::LoadFromSaveData(
	const FInventorySaveData& SaveData)
{
	APlayerController* PC = Cast<APlayerController>(GetOwner());
	APlayerState* PS = PC ? PC->GetPlayerState<APlayerState>() : nullptr;
	UAbilitySystemComponent* ASC = PS ? PS->FindComponentByClass<UAbilitySystemComponent>() : nullptr;

	// 1. 기존 GE 제거
	if (ASC)
	{
		for (auto It = ActiveGEMap.CreateIterator(); It; ++It)
		{
			if (It.Value().IsValid())
			{
				ASC->RemoveActiveGameplayEffect(It.Value());
			}
		}
	}

	ActiveGEMap.Empty();

	InventoryList.Entries.Empty();
	InventoryList.MarkArrayDirty();

	EquipSlots.SetNum(3);

	// =========================
	// Instance 직접 매핑
	// =========================
	TArray<ULyraInventoryItemInstance*> CreatedItems;
	CreatedItems.Reserve(SaveData.Items.Num());

	// 1. 아이템 생성
	for (const FInventoryEntrySave& Data : SaveData.Items)
	{
		if (!Data.ItemDef) continue;

		ULyraInventoryItemInstance* NewItem =
			AddItemDefinition(
				Data.ItemDef,
				Data.StackCount,
				Data.RandomSeed,
				Data.OptionType,
				Data.Rarity
			);

		if (NewItem)
		{
			CreatedItems.Add(NewItem);
		}
	}

	// 2. 장착 복구
	for (int32 i = 0; i < SaveData.Items.Num(); ++i)
	{
		const FInventoryEntrySave& Data = SaveData.Items[i];

		if (Data.EquipSlotIndex == INDEX_NONE)
			continue;

		if (!CreatedItems.IsValidIndex(i))
			continue;

		ULyraInventoryItemInstance* Item = CreatedItems[i];

		if (Item)
		{
			EquipFromInventory(Data.EquipSlotIndex, Item);
		}
	}

	OnEquipChanged.Broadcast();
}

void ULyraInventoryManagerComponent::BroadcastItemAdded(
	ULyraInventoryItemInstance* Instance,
	int32 Count)
{
	if (!Instance)
		return;

	UWorld* World = GetWorld();
	if (!World)
		return;

	FLyraInventoryChangeMessage Message;

	Message.InventoryOwner = this;
	Message.Instance = Instance;
	Message.NewCount = Count;
	Message.Delta = Count;

	// 여기서 확정값 전달
	Message.Rarity = Instance->Rarity;
	Message.OptionType = Instance->OptionType;

	UGameplayMessageSubsystem::Get(World)
		.BroadcastMessage(
			TAG_Lyra_Inventory_Message_StackChanged,
			Message
		);
}

void ULyraInventoryManagerComponent::SaveInventory()
{
	ALyraPlayerController* PC =
		Cast<ALyraPlayerController>(GetOwner());

	if (!PC)
	{
		return;
	}

	UWorld* World = GetWorld();

	if (!World)
	{
		return;
	}

	UGameInstance* GI = World->GetGameInstance();

	if (!GI)
	{
		return;
	}

	UInventorySaveSubsystem* SaveSys =
		GI->GetSubsystem<UInventorySaveSubsystem>();

	if (!SaveSys)
	{
		return;
	}

	const FString PlayerId =
		PC->GetInventorySavePlayerId();

	if (PlayerId.IsEmpty())
	{
		UE_LOG(LogInventorySave, Warning,
			TEXT("Save Failed - Empty PlayerId"));

		return;
	}

	const FInventorySaveData SaveData =
		MakeSaveData();

	SaveSys->SetInventory(PlayerId, SaveData);

	SaveSys->RequestSave();

	UE_LOG(LogInventorySave, Log,
		TEXT("Inventory Saved"));
}

//////////////////////////////////////////////////////////////////////
//

// UCLASS(Abstract)
// class ULyraInventoryFilter : public UObject
// {
// public:
// 	virtual bool PassesFilter(ULyraInventoryItemInstance* Instance) const { return true; }
// };

// UCLASS()
// class ULyraInventoryFilter_HasTag : public ULyraInventoryFilter
// {
// public:
// 	virtual bool PassesFilter(ULyraInventoryItemInstance* Instance) const { return true; }
// };
