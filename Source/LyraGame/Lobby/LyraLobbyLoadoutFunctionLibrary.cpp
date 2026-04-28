// Copyright Epic Games, Inc. All Rights Reserved.

#include "Lobby/LyraLobbyLoadoutFunctionLibrary.h"

#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Lobby/LyraLobbyPlayerStateComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraLobbyLoadoutFunctionLibrary)

FPrimaryAssetId ULyraLobbyLoadoutFunctionLibrary::MakeLobbyPrimaryAssetId(FName PrimaryAssetType, FName PrimaryAssetName)
{
	return FPrimaryAssetId(FPrimaryAssetType(PrimaryAssetType), PrimaryAssetName);
}

ULyraLobbyPlayerStateComponent* ULyraLobbyLoadoutFunctionLibrary::GetLocalLobbyPlayerStateComponent(const UObject* WorldContextObject, int32 LocalPlayerIndex)
{
	const APlayerController* PlayerController = UGameplayStatics::GetPlayerController(WorldContextObject, LocalPlayerIndex);
	const APlayerState* PlayerState = PlayerController ? PlayerController->PlayerState : nullptr;
	return PlayerState ? PlayerState->FindComponentByClass<ULyraLobbyPlayerStateComponent>() : nullptr;
}

bool ULyraLobbyLoadoutFunctionLibrary::GetLocalLobbyLoadout(const UObject* WorldContextObject, FLyraLobbyPlayerLoadout& OutLoadout, int32 LocalPlayerIndex)
{
	if (const ULyraLobbyPlayerStateComponent* LobbyPlayer = GetLocalLobbyPlayerStateComponent(WorldContextObject, LocalPlayerIndex))
	{
		OutLoadout = LobbyPlayer->GetLobbyLoadout();
		return true;
	}

	OutLoadout = FLyraLobbyPlayerLoadout();
	return false;
}

bool ULyraLobbyLoadoutFunctionLibrary::SubmitLocalLobbyLoadout(const UObject* WorldContextObject, const FLyraLobbyPlayerLoadout& Loadout, int32 LocalPlayerIndex)
{
	if (ULyraLobbyPlayerStateComponent* LobbyPlayer = GetLocalLobbyPlayerStateComponent(WorldContextObject, LocalPlayerIndex))
	{
		LobbyPlayer->SubmitLobbyLoadout(Loadout);
		return true;
	}

	return false;
}

bool ULyraLobbyLoadoutFunctionLibrary::SetLocalLobbyReady(const UObject* WorldContextObject, bool bReady, int32 LocalPlayerIndex)
{
	if (ULyraLobbyPlayerStateComponent* LobbyPlayer = GetLocalLobbyPlayerStateComponent(WorldContextObject, LocalPlayerIndex))
	{
		LobbyPlayer->SetLobbyReady(bReady);
		return true;
	}

	return false;
}

FLyraLobbyPlayerLoadout ULyraLobbyLoadoutFunctionLibrary::SetCharacterPreset(FLyraLobbyPlayerLoadout Loadout, FPrimaryAssetId CharacterPresetId)
{
	Loadout.CharacterPresetId = CharacterPresetId;
	return Loadout;
}

FLyraLobbyPlayerLoadout ULyraLobbyLoadoutFunctionLibrary::SetDoctrinePreset(FLyraLobbyPlayerLoadout Loadout, FPrimaryAssetId DoctrinePresetId)
{
	Loadout.DoctrinePresetId = DoctrinePresetId;
	return Loadout;
}

FLyraLobbyPlayerLoadout ULyraLobbyLoadoutFunctionLibrary::SetActiveCombatStyle(FLyraLobbyPlayerLoadout Loadout, FPrimaryAssetId ActiveCombatStyleId)
{
	Loadout.ActiveCombatStyleId = ActiveCombatStyleId;
	return Loadout;
}

FLyraLobbyPlayerLoadout ULyraLobbyLoadoutFunctionLibrary::AddUniqueEquipmentPart(FLyraLobbyPlayerLoadout Loadout, FPrimaryAssetId EquipmentPartId)
{
	if (EquipmentPartId.IsValid())
	{
		Loadout.EquipmentPartIds.AddUnique(EquipmentPartId);
	}

	return Loadout;
}

FLyraLobbyPlayerLoadout ULyraLobbyLoadoutFunctionLibrary::RemoveEquipmentPart(FLyraLobbyPlayerLoadout Loadout, FPrimaryAssetId EquipmentPartId)
{
	Loadout.EquipmentPartIds.Remove(EquipmentPartId);
	return Loadout;
}

FLyraLobbyPlayerLoadout ULyraLobbyLoadoutFunctionLibrary::ClearEquipmentParts(FLyraLobbyPlayerLoadout Loadout)
{
	Loadout.EquipmentPartIds.Reset();
	return Loadout;
}

FLyraLobbyPlayerLoadout ULyraLobbyLoadoutFunctionLibrary::SetAccessorySlot(FLyraLobbyPlayerLoadout Loadout, FGameplayTag SlotTag, FPrimaryAssetId AccessoryId)
{
	if (!SlotTag.IsValid())
	{
		return Loadout;
	}

	for (FLyraLobbyAccessorySelection& AccessorySelection : Loadout.AccessorySlots)
	{
		if (AccessorySelection.SlotTag == SlotTag)
		{
			AccessorySelection.AccessoryId = AccessoryId;
			return Loadout;
		}
	}

	FLyraLobbyAccessorySelection& NewSelection = Loadout.AccessorySlots.AddDefaulted_GetRef();
	NewSelection.SlotTag = SlotTag;
	NewSelection.AccessoryId = AccessoryId;
	return Loadout;
}

FLyraLobbyPlayerLoadout ULyraLobbyLoadoutFunctionLibrary::ClearAccessorySlot(FLyraLobbyPlayerLoadout Loadout, FGameplayTag SlotTag)
{
	Loadout.AccessorySlots.RemoveAll([SlotTag](const FLyraLobbyAccessorySelection& AccessorySelection)
	{
		return AccessorySelection.SlotTag == SlotTag;
	});

	return Loadout;
}
