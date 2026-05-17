#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "InventoryFragment_EquipEffect.h"
#include "InventorySaveSubsystem.generated.h"

class ULyraInventoryItemDefinition;

// ============================
// 개별 아이템 저장 데이터
// ============================
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

	// 추가
	UPROPERTY()
	EItemOptionType OptionType = EItemOptionType::Attack;
};

// ============================
// 전체 인벤토리 저장 데이터
// ============================
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

	void SetInventory(
		const FString& PlayerId,
		const FInventorySaveData& Data);

	bool GetInventory(
		const FString& PlayerId,
		FInventorySaveData& OutData) const;

private:

	UPROPERTY()
	TMap<FString, FInventorySaveData> SavedInventories;
};