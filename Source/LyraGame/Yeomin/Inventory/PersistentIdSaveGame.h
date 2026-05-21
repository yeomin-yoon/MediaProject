// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "PersistentIdSaveGame.generated.h"

/**
 * 
 */
UCLASS()
class LYRAGAME_API UPersistentIdSaveGame : public USaveGame
{
	GENERATED_BODY()
	
public:
	UPROPERTY()
	FString SavedId;
};
