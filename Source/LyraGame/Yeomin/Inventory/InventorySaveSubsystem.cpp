// Fill out your copyright notice in the Description page of Project Settings.


#include "InventorySaveSubsystem.h"

#include "InventoryLogChannels.h"
#include "InventorySaveGame.h"
#include "Kismet/GameplayStatics.h"

void UInventorySaveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	SaveSlotName = GetSaveSlotName();
	
	LoadAllFromDisk();
}

void UInventorySaveSubsystem::TryInitializeLoad()
{
	if (bHasInitializedLoad)
		return;

	LoadAllFromDisk();
	bHasInitializedLoad = true;
}

void UInventorySaveSubsystem::SetInventory(
	const FString& PlayerId,
	const FInventorySaveData& Data)
{
	if (PlayerId.IsEmpty())
	{
		UE_LOG(LogInventorySave, Warning,
			TEXT("[Save] SetInventory Failed - Empty PlayerId"));
		return;
	}

	UE_LOG(LogInventorySave, Log,
		TEXT("[Save] SetInventory PlayerId=%s Items=%d"),
		*PlayerId,
		Data.Items.Num());

	// 기존 데이터 덮어쓰기
	SavedInventories.FindOrAdd(PlayerId) = Data;

	RequestSave();
}

bool UInventorySaveSubsystem::GetInventory(
	const FString& PlayerId,
	FInventorySaveData& OutData) const
{
	if (const FInventorySaveData* Found =
		SavedInventories.Find(PlayerId))
	{
		OutData = *Found;
		return true;
	}

	return false;
}

bool UInventorySaveSubsystem::HasInventory(const FString& PlayerId) const
{
	if (PlayerId.IsEmpty())
		return false;

	return SavedInventories.Contains(PlayerId);
}

void UInventorySaveSubsystem::ClearInventory(const FString& PlayerId)
{
	if (PlayerId.IsEmpty())
		return;

	SavedInventories.Remove(PlayerId);
}

void UInventorySaveSubsystem::ClearAllInventories()
{
	SavedInventories.Empty();
}

bool UInventorySaveSubsystem::SaveInventoryToDisk(const FString& PlayerId)
{
	if (PlayerId.IsEmpty())
		return false;

	if (!SavedInventories.Contains(PlayerId))
		return false;

	return SaveAllToDisk();
}

bool UInventorySaveSubsystem::LoadInventoryFromDisk(
	const FString& PlayerId,
	FInventorySaveData& OutData)
{
	if (PlayerId.IsEmpty())
		return false;

	if (!LoadAllFromDisk())
		return false;

	return GetInventory(PlayerId, OutData);
}

bool UInventorySaveSubsystem::SaveAllToDisk()
{
	UInventorySaveGame* SaveGameObject =
		Cast<UInventorySaveGame>(
			UGameplayStatics::CreateSaveGameObject(
				UInventorySaveGame::StaticClass()));

	if (!SaveGameObject)
	{
		return false;
	}

	SaveGameObject->SavedInventories = SavedInventories;

	const bool bSuccess =
		UGameplayStatics::SaveGameToSlot(
			SaveGameObject,
			SaveSlotName,
			UserIndex);

	if (bSuccess)
	{
		UE_LOG(LogInventorySave, Log,
			TEXT("Save Success Slot=%s"),
			*SaveSlotName);
	}
	else
	{
		UE_LOG(LogInventorySave, Error,
			TEXT("Save Failed Slot=%s"),
			*SaveSlotName);
	}

	for (const auto& Pair : SavedInventories)
	{
		UE_LOG(LogInventorySave, Verbose,
			TEXT("Saved Inventory PlayerId=%s ItemCount=%d"),
			*Pair.Key,
			Pair.Value.Items.Num());
	}

	return bSuccess;
}

bool UInventorySaveSubsystem::LoadAllFromDisk()
{
	if (!UGameplayStatics::DoesSaveGameExist(
		SaveSlotName,
		UserIndex))
	{
		UE_LOG(LogInventorySave, Log,
			TEXT("No Save File"));

		return false;
	}

	UInventorySaveGame* LoadedSave =
		Cast<UInventorySaveGame>(
			UGameplayStatics::LoadGameFromSlot(
				SaveSlotName,
				UserIndex));

	if (!LoadedSave)
	{
		UE_LOG(LogInventorySave, Error,
			TEXT("Load Failed"));

		return false;
	}

	UE_LOG(LogInventorySave, Verbose,
		TEXT("Before Apply Num=%d"),
		SavedInventories.Num());
	
	// ============================================================
	// 핵심
	// ============================================================

	SavedInventories = LoadedSave->SavedInventories;

	UE_LOG(LogInventorySave, Log,
		TEXT("Load Complete InventoryCount=%d"),
		SavedInventories.Num());

	for (const auto& Pair : SavedInventories)
	{
		UE_LOG(LogInventorySave, Verbose,
			TEXT("Loaded Inventory PlayerId=%s ItemCount=%d"),
			*Pair.Key,
			Pair.Value.Items.Num());
	}

	return true;
}

void UInventorySaveSubsystem::RequestSave()
{
	bInventoryDirty = true;

	UWorld* World = GetWorld();

	if (!World)
	{
		return;
	}

	// 이미 예약 중이면 무시
	if (World->GetTimerManager().IsTimerActive(SaveTimerHandle))
	{
		return;
	}

	World->GetTimerManager().SetTimer(
		SaveTimerHandle,
		this,
		&UInventorySaveSubsystem::FlushSave,
		5.0f,
		false);
}

void UInventorySaveSubsystem::FlushSave()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SaveTimerHandle);
	}

	if (!bInventoryDirty)
	{
		return;
	}

	bInventoryDirty = false;

	UE_LOG(LogInventorySave, Log,
		TEXT("FlushSave Start"));

	SaveAllToDisk();
}

FString UInventorySaveSubsystem::GetSaveSlotName() const
{
#if WITH_EDITOR
	int32 PIEId = UE::GetPlayInEditorID();

	// PIE 초기화 전이면 안전 fallback
	if (PIEId < 0)
	{
		return TEXT("InventorySave_PIE_Fallback");
	}

	return FString(TEXT("InventorySave_PIE_")) + FString::FromInt(PIEId);
#else
	return TEXT("InventorySave");
#endif
}