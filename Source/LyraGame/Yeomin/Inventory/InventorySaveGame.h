// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "InventorySaveSubsystem.h"
#include "InventorySaveGame.generated.h"

UCLASS()
class LYRAGAME_API UInventorySaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TMap<FString, FInventorySaveData> SavedInventories;
};
