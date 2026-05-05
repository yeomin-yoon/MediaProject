// Fill out your copyright notice in the Description page of Project Settings.


#include "InventorySaveSubsystem.h"

void UInventorySaveSubsystem::SetInventory(const FString& PlayerId, const FInventorySaveData& Data)
{
	SavedInventories.Add(PlayerId, Data);
}

bool UInventorySaveSubsystem::GetInventory(const FString& PlayerId, FInventorySaveData& OutData) const
{
	if (const FInventorySaveData* Found = SavedInventories.Find(PlayerId))
	{
		OutData = *Found;
		return true;
	}
	return false;
}