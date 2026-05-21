#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Inventory/LyraInventoryItemInstance.h"
#include "InventorySaveSubsystem.generated.h"

class ULyraInventoryItemDefinition;

USTRUCT(BlueprintType)
struct FInventoryEntrySave
{
	GENERATED_BODY()

	UPROPERTY()
	TSubclassOf<ULyraInventoryItemDefinition> ItemDef;

	UPROPERTY()
	int32 StackCount = 1;

	UPROPERTY()
	int32 EquipSlotIndex = INDEX_NONE;

	UPROPERTY()
	int32 RandomSeed = 0;

	UPROPERTY()
	EItemOptionType OptionType = EItemOptionType::Attack;

	UPROPERTY()
	EItemRarity Rarity = EItemRarity::Common;
};

USTRUCT(BlueprintType)
struct FInventorySaveData
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FInventoryEntrySave> Items;
};

UCLASS()
class LYRAGAME_API UInventorySaveSubsystem
	: public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	void TryInitializeLoad();

	void SetInventory(const FString& PlayerId, const FInventorySaveData& Data);
	bool GetInventory(const FString& PlayerId, FInventorySaveData& OutData) const;
	bool HasInventory(const FString& PlayerId) const;
	void ClearInventory(const FString& PlayerId);
	void ClearAllInventories();

	bool SaveInventoryToDisk(const FString& PlayerId);
	bool LoadInventoryFromDisk(const FString& PlayerId, FInventorySaveData& OutData);
	bool SaveAllToDisk();
	bool LoadAllFromDisk();

	void RequestSave();

private:
	void FlushSave();
	bool bInventoryDirty = false;
	FTimerHandle SaveTimerHandle;
	FString GetSaveSlotName() const;

private:
	UPROPERTY()
	TMap<FString, FInventorySaveData> SavedInventories;

	UPROPERTY()
	FString SaveSlotName = TEXT("InventorySave");

	UPROPERTY()
	int32 UserIndex = 0;
	
	bool bHasInitializedLoad = false;
};