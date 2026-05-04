// Copyright Epic Games, Inc. All Rights Reserved.

#include "Lobby/LyraLobbyLoadoutFunctionLibrary.h"

#include "CommonSessionSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameModes/LyraUserFacingExperienceDefinition.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Lobby/LyraLobbyLocalLoadoutSubsystem.h"
#include "Lobby/LyraLobbyPlayerStateComponent.h"
#include "Player/LyraPlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraLobbyLoadoutFunctionLibrary)

namespace LyraLobbyLoadoutFunctionLibrary_Private
{
	UCommonSession_HostSessionRequest* CreateRequestForExperience(const UObject* WorldContextObject, const ULyraUserFacingExperienceDefinition* UserFacingExperience, ECommonSessionOnlineMode OnlineMode)
	{
		if (!UserFacingExperience)
		{
			return nullptr;
		}

		UCommonSession_HostSessionRequest* Request = UserFacingExperience->CreateHostingRequest(WorldContextObject);
		if (Request)
		{
			Request->OnlineMode = OnlineMode;
		}

		return Request;
	}
}

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
	SaveLocalLobbyLoadoutForTravel(WorldContextObject, Loadout, LocalPlayerIndex);

	if (ULyraLobbyPlayerStateComponent* LobbyPlayer = GetLocalLobbyPlayerStateComponent(WorldContextObject, LocalPlayerIndex))
	{
		LobbyPlayer->SubmitLobbyLoadout(Loadout);
		return true;
	}

	return false;
}

bool ULyraLobbyLoadoutFunctionLibrary::SaveLocalLobbyLoadoutForTravel(const UObject* WorldContextObject, const FLyraLobbyPlayerLoadout& Loadout, int32 LocalPlayerIndex)
{
	UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(WorldContextObject);
	ULyraLobbyLocalLoadoutSubsystem* LocalLoadoutSubsystem = GameInstance ? GameInstance->GetSubsystem<ULyraLobbyLocalLoadoutSubsystem>() : nullptr;
	if (!LocalLoadoutSubsystem)
	{
		return false;
	}

	LocalLoadoutSubsystem->SaveLocalLoadout(LocalPlayerIndex, Loadout);
	return true;
}

bool ULyraLobbyLoadoutFunctionLibrary::PushSavedLocalLobbyLoadoutToServer(const UObject* WorldContextObject, int32 LocalPlayerIndex)
{
	UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(WorldContextObject);
	ULyraLobbyLocalLoadoutSubsystem* LocalLoadoutSubsystem = GameInstance ? GameInstance->GetSubsystem<ULyraLobbyLocalLoadoutSubsystem>() : nullptr;
	if (!LocalLoadoutSubsystem)
	{
		return false;
	}

	FLyraLobbyPlayerLoadout SavedLoadout;
	if (!LocalLoadoutSubsystem->GetLocalLoadout(LocalPlayerIndex, SavedLoadout))
	{
		return false;
	}

	if (ULyraLobbyPlayerStateComponent* LobbyPlayer = GetLocalLobbyPlayerStateComponent(WorldContextObject, LocalPlayerIndex))
	{
		LobbyPlayer->SubmitLobbyLoadout(SavedLoadout);
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

bool ULyraLobbyLoadoutFunctionLibrary::HostLocalLobbyExperience(const UObject* WorldContextObject, const ULyraUserFacingExperienceDefinition* UserFacingExperience, int32 LocalPlayerIndex, ECommonSessionOnlineMode OnlineMode)
{
	APlayerController* HostingPlayer = UGameplayStatics::GetPlayerController(WorldContextObject, LocalPlayerIndex);
	UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(WorldContextObject);
	UCommonSessionSubsystem* SessionSubsystem = GameInstance ? GameInstance->GetSubsystem<UCommonSessionSubsystem>() : nullptr;
	UCommonSession_HostSessionRequest* Request = LyraLobbyLoadoutFunctionLibrary_Private::CreateRequestForExperience(WorldContextObject, UserFacingExperience, OnlineMode);
	if (!HostingPlayer || !SessionSubsystem || !Request)
	{
		return false;
	}

	FText Error;
	if (!Request->ValidateAndLogErrors(Error))
	{
		return false;
	}

	SessionSubsystem->HostSession(HostingPlayer, Request);
	return true;
}

bool ULyraLobbyLoadoutFunctionLibrary::QuickPlayLocalLobbyExperience(const UObject* WorldContextObject, const ULyraUserFacingExperienceDefinition* UserFacingExperience, int32 LocalPlayerIndex, ECommonSessionOnlineMode OnlineMode)
{
	if (!UserFacingExperience)
	{
		return false;
	}

	APlayerController* JoiningOrHostingPlayer = UGameplayStatics::GetPlayerController(WorldContextObject, LocalPlayerIndex);
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (World && World->GetNetMode() != NM_Standalone)
	{
		if (ALyraPlayerController* LyraPlayerController = Cast<ALyraPlayerController>(JoiningOrHostingPlayer))
		{
			LyraPlayerController->RequestConnectedLobbyReadyToTravelToExperience(UserFacingExperience);
			return true;
		}
	}

	UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(WorldContextObject);
	UCommonSessionSubsystem* SessionSubsystem = GameInstance ? GameInstance->GetSubsystem<UCommonSessionSubsystem>() : nullptr;
	UCommonSession_HostSessionRequest* Request = LyraLobbyLoadoutFunctionLibrary_Private::CreateRequestForExperience(WorldContextObject, UserFacingExperience, OnlineMode);
	if (!JoiningOrHostingPlayer || !SessionSubsystem || !Request)
	{
		return false;
	}

	FText Error;
	if (!Request->ValidateAndLogErrors(Error))
	{
		return false;
	}

	SessionSubsystem->QuickPlaySession(JoiningOrHostingPlayer, Request);
	return true;
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
