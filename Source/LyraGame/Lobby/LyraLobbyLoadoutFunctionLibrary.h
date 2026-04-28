// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "Lobby/LyraLobbyTypes.h"

#include "LyraLobbyLoadoutFunctionLibrary.generated.h"

class ULyraLobbyPlayerStateComponent;
class UObject;

UCLASS()
class LYRAGAME_API ULyraLobbyLoadoutFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lyra|Lobby")
	static FPrimaryAssetId MakeLobbyPrimaryAssetId(FName PrimaryAssetType, FName PrimaryAssetName);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lyra|Lobby", meta = (WorldContext = "WorldContextObject"))
	static ULyraLobbyPlayerStateComponent* GetLocalLobbyPlayerStateComponent(const UObject* WorldContextObject, int32 LocalPlayerIndex);

	UFUNCTION(BlueprintCallable, Category = "Lyra|Lobby", meta = (WorldContext = "WorldContextObject"))
	static bool GetLocalLobbyLoadout(const UObject* WorldContextObject, FLyraLobbyPlayerLoadout& OutLoadout, int32 LocalPlayerIndex);

	UFUNCTION(BlueprintCallable, Category = "Lyra|Lobby", meta = (WorldContext = "WorldContextObject"))
	static bool SubmitLocalLobbyLoadout(const UObject* WorldContextObject, const FLyraLobbyPlayerLoadout& Loadout, int32 LocalPlayerIndex);

	UFUNCTION(BlueprintCallable, Category = "Lyra|Lobby", meta = (WorldContext = "WorldContextObject"))
	static bool SetLocalLobbyReady(const UObject* WorldContextObject, bool bReady, int32 LocalPlayerIndex);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lyra|Lobby")
	static FLyraLobbyPlayerLoadout SetCharacterPreset(FLyraLobbyPlayerLoadout Loadout, FPrimaryAssetId CharacterPresetId);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lyra|Lobby")
	static FLyraLobbyPlayerLoadout SetDoctrinePreset(FLyraLobbyPlayerLoadout Loadout, FPrimaryAssetId DoctrinePresetId);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lyra|Lobby")
	static FLyraLobbyPlayerLoadout SetActiveCombatStyle(FLyraLobbyPlayerLoadout Loadout, FPrimaryAssetId ActiveCombatStyleId);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lyra|Lobby")
	static FLyraLobbyPlayerLoadout AddUniqueEquipmentPart(FLyraLobbyPlayerLoadout Loadout, FPrimaryAssetId EquipmentPartId);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lyra|Lobby")
	static FLyraLobbyPlayerLoadout RemoveEquipmentPart(FLyraLobbyPlayerLoadout Loadout, FPrimaryAssetId EquipmentPartId);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lyra|Lobby")
	static FLyraLobbyPlayerLoadout ClearEquipmentParts(FLyraLobbyPlayerLoadout Loadout);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lyra|Lobby")
	static FLyraLobbyPlayerLoadout SetAccessorySlot(FLyraLobbyPlayerLoadout Loadout, FGameplayTag SlotTag, FPrimaryAssetId AccessoryId);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lyra|Lobby")
	static FLyraLobbyPlayerLoadout ClearAccessorySlot(FLyraLobbyPlayerLoadout Loadout, FGameplayTag SlotTag);
};
