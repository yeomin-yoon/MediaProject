// Copyright Epic Games, Inc. All Rights Reserved.

#include "Lobby/LyraLobbyLocalLoadoutSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraLobbyLocalLoadoutSubsystem)

void ULyraLobbyLocalLoadoutSubsystem::SaveLocalLoadout(int32 LocalPlayerIndex, const FLyraLobbyPlayerLoadout& Loadout)
{
	if (LocalPlayerIndex < 0)
	{
		return;
	}

	EnsureSlot(LocalPlayerIndex);
	LocalLoadouts[LocalPlayerIndex].bHasLoadout = true;
	LocalLoadouts[LocalPlayerIndex].Loadout = Loadout;
}

bool ULyraLobbyLocalLoadoutSubsystem::GetLocalLoadout(int32 LocalPlayerIndex, FLyraLobbyPlayerLoadout& OutLoadout) const
{
	if (LocalPlayerIndex >= 0 && LocalLoadouts.IsValidIndex(LocalPlayerIndex) && LocalLoadouts[LocalPlayerIndex].bHasLoadout)
	{
		OutLoadout = LocalLoadouts[LocalPlayerIndex].Loadout;
		return true;
	}

	OutLoadout = FLyraLobbyPlayerLoadout();
	return false;
}

void ULyraLobbyLocalLoadoutSubsystem::ClearLocalLoadout(int32 LocalPlayerIndex)
{
	if (LocalLoadouts.IsValidIndex(LocalPlayerIndex))
	{
		LocalLoadouts[LocalPlayerIndex] = FLyraLocalLobbyLoadoutSlot();
	}
}

void ULyraLobbyLocalLoadoutSubsystem::EnsureSlot(int32 LocalPlayerIndex)
{
	if (LocalPlayerIndex >= LocalLoadouts.Num())
	{
		LocalLoadouts.SetNum(LocalPlayerIndex + 1);
	}
}
