// Copyright Epic Games, Inc. All Rights Reserved.

#include "Lobby/LyraLobbyStateComponent.h"

#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "Lobby/LyraLobbyPlayerStateComponent.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraLobbyStateComponent)

class FLifetimeProperty;

ULyraLobbyStateComponent::ULyraLobbyStateComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
}

void ULyraLobbyStateComponent::SetLobbyPhase(ELyraLobbyPhase NewPhase)
{
	if (AActor* Owner = GetOwner(); Owner && Owner->HasAuthority() && LobbyPhase != NewPhase)
	{
		LobbyPhase = NewPhase;
		OnRep_LobbyPhase();
	}
}

void ULyraLobbyStateComponent::ConfigureExpedition(const FLyraExpeditionSessionConfig& NewConfig)
{
	if (AActor* Owner = GetOwner(); Owner && Owner->HasAuthority())
	{
		ExpeditionConfig = NewConfig;
		++ExpeditionConfig.Revision;
		OnRep_ExpeditionConfig();
	}
}

int32 ULyraLobbyStateComponent::LockReadyPlayers()
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return 0;
	}

	int32 LockedCount = 0;

	if (const AGameStateBase* GameState = GetGameState<AGameStateBase>())
	{
		for (APlayerState* PlayerState : GameState->PlayerArray)
		{
			if (ULyraLobbyPlayerStateComponent* LobbyPlayer = PlayerState ? PlayerState->FindComponentByClass<ULyraLobbyPlayerStateComponent>() : nullptr)
			{
				if (LobbyPlayer->GetReadyState() == ELyraLobbyReadyState::Ready)
				{
					LobbyPlayer->LockLobbyLoadout();
					++LockedCount;
				}
			}
		}
	}

	if (LockedCount > 0)
	{
		SetLobbyPhase(ELyraLobbyPhase::Locked);
	}

	return LockedCount;
}

void ULyraLobbyStateComponent::ResetLobby()
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}

	if (const AGameStateBase* GameState = GetGameState<AGameStateBase>())
	{
		for (APlayerState* PlayerState : GameState->PlayerArray)
		{
			if (ULyraLobbyPlayerStateComponent* LobbyPlayer = PlayerState ? PlayerState->FindComponentByClass<ULyraLobbyPlayerStateComponent>() : nullptr)
			{
				LobbyPlayer->UnlockLobbyLoadout();
			}
		}
	}

	SetLobbyPhase(ELyraLobbyPhase::Open);
}

int32 ULyraLobbyStateComponent::GetReadyPlayerCount() const
{
	int32 ReadyCount = 0;

	if (const AGameStateBase* GameState = GetGameState<AGameStateBase>())
	{
		for (APlayerState* PlayerState : GameState->PlayerArray)
		{
			if (const ULyraLobbyPlayerStateComponent* LobbyPlayer = PlayerState ? PlayerState->FindComponentByClass<ULyraLobbyPlayerStateComponent>() : nullptr)
			{
				if (LobbyPlayer->IsReadyOrLocked())
				{
					++ReadyCount;
				}
			}
		}
	}

	return ReadyCount;
}

int32 ULyraLobbyStateComponent::GetLockedPlayerCount() const
{
	int32 LockedCount = 0;

	if (const AGameStateBase* GameState = GetGameState<AGameStateBase>())
	{
		for (APlayerState* PlayerState : GameState->PlayerArray)
		{
			if (const ULyraLobbyPlayerStateComponent* LobbyPlayer = PlayerState ? PlayerState->FindComponentByClass<ULyraLobbyPlayerStateComponent>() : nullptr)
			{
				if (LobbyPlayer->GetReadyState() == ELyraLobbyReadyState::Locked)
				{
					++LockedCount;
				}
			}
		}
	}

	return LockedCount;
}

bool ULyraLobbyStateComponent::AreAllPlayersReady() const
{
	if (const AGameStateBase* GameState = GetGameState<AGameStateBase>())
	{
		if (GameState->PlayerArray.IsEmpty())
		{
			return false;
		}

		for (APlayerState* PlayerState : GameState->PlayerArray)
		{
			const ULyraLobbyPlayerStateComponent* LobbyPlayer = PlayerState ? PlayerState->FindComponentByClass<ULyraLobbyPlayerStateComponent>() : nullptr;
			if (!LobbyPlayer || !LobbyPlayer->IsReadyOrLocked())
			{
				return false;
			}
		}

		return true;
	}

	return false;
}

void ULyraLobbyStateComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, LobbyPhase);
	DOREPLIFETIME(ThisClass, ExpeditionConfig);
}

void ULyraLobbyStateComponent::OnRep_LobbyPhase()
{
	OnLobbyPhaseChanged.Broadcast(LobbyPhase);
}

void ULyraLobbyStateComponent::OnRep_ExpeditionConfig()
{
	OnExpeditionConfigChanged.Broadcast(ExpeditionConfig);
}
