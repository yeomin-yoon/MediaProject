// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Lobby/LyraLobbyTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "LyraLobbyLocalLoadoutSubsystem.generated.h"

USTRUCT(BlueprintType)
struct FLyraLocalLobbyLoadoutSlot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Lyra|Lobby")
	bool bHasLoadout = false;

	UPROPERTY(BlueprintReadWrite, Category = "Lyra|Lobby")
	FLyraLobbyPlayerLoadout Loadout;
};

UCLASS()
class LYRAGAME_API ULyraLobbyLocalLoadoutSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Lyra|Lobby")
	void SaveLocalLoadout(int32 LocalPlayerIndex, const FLyraLobbyPlayerLoadout& Loadout);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lyra|Lobby")
	bool GetLocalLoadout(int32 LocalPlayerIndex, FLyraLobbyPlayerLoadout& OutLoadout) const;

	UFUNCTION(BlueprintCallable, Category = "Lyra|Lobby")
	void ClearLocalLoadout(int32 LocalPlayerIndex);

private:
	void EnsureSlot(int32 LocalPlayerIndex);

private:
	UPROPERTY()
	TArray<FLyraLocalLobbyLoadoutSlot> LocalLoadouts;
};
