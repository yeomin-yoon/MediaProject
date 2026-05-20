// Fill out your copyright notice in the Description page of Project Settings.


#include "InventorySaveSubsystem.h"

#include "InventorySaveGame.h"
#include "Kismet/GameplayStatics.h"

void UInventorySaveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	SaveSlotName = MakeSaveSlotName();

	LoadAllFromDisk();
}

void UInventorySaveSubsystem::SetInventory(const FString& PlayerId, const FInventorySaveData& Data)
{
	if (PlayerId.IsEmpty())
		return;

	SavedInventories.Add(PlayerId, Data);
}

bool UInventorySaveSubsystem::GetInventory(const FString& PlayerId, FInventorySaveData& OutData) const
{
	if (PlayerId.IsEmpty())
		return false;

	if (const FInventorySaveData* Found = SavedInventories.Find(PlayerId))
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
	UInventorySaveGame* SaveGameObject = Cast<UInventorySaveGame>(
		UGameplayStatics::CreateSaveGameObject(UInventorySaveGame::StaticClass()));

	if (!SaveGameObject)
		return false;

	SaveGameObject->SavedInventories = SavedInventories;

	return UGameplayStatics::SaveGameToSlot(
		SaveGameObject,
		SaveSlotName,
		UserIndex);
}

bool UInventorySaveSubsystem::LoadAllFromDisk()
{
	if (!UGameplayStatics::DoesSaveGameExist(SaveSlotName, UserIndex))
		return false;

	UInventorySaveGame* LoadedSave = Cast<UInventorySaveGame>(
		UGameplayStatics::LoadGameFromSlot(SaveSlotName, UserIndex));

	if (!LoadedSave)
		return false;

	SavedInventories = LoadedSave->SavedInventories;

	return true;
}

FString UInventorySaveSubsystem::MakeSaveSlotName()
{
	return TEXT("InventorySave");
}