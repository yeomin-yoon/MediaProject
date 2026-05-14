// Copyright Epic Games, Inc. All Rights Reserved.

#include "Lobby/LyraLobbyPlayerStateComponent.h"

#include "GameFramework/Actor.h"
#include "LyraLogChannels.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraLobbyPlayerStateComponent)

class FLifetimeProperty;

ULyraLobbyPlayerStateComponent::ULyraLobbyPlayerStateComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
}

void ULyraLobbyPlayerStateComponent::SubmitLobbyLoadout(const FLyraLobbyPlayerLoadout& NewLoadout)
{
	if (HasAuthorityOwner())
	{
		ApplySubmittedLoadout(NewLoadout);
	}
	else
	{
		ServerSubmitLobbyLoadout(NewLoadout);
	}
}

void ULyraLobbyPlayerStateComponent::SetLobbyReady(bool bReady)
{
	if (HasAuthorityOwner())
	{
		ApplyReadyState(bReady ? ELyraLobbyReadyState::Ready : ELyraLobbyReadyState::Editing);
	}
	else
	{
		ServerSetLobbyReady(bReady);
	}
}

void ULyraLobbyPlayerStateComponent::LockLobbyLoadout()
{
	if (HasAuthorityOwner())
	{
		ApplyReadyState(ELyraLobbyReadyState::Locked);
	}
}

void ULyraLobbyPlayerStateComponent::UnlockLobbyLoadout()
{
	if (HasAuthorityOwner())
	{
		ApplyReadyState(ELyraLobbyReadyState::Editing);
	}
}

bool ULyraLobbyPlayerStateComponent::IsReadyOrLocked() const
{
	return ReadyState == ELyraLobbyReadyState::Ready || ReadyState == ELyraLobbyReadyState::Locked;
}

void ULyraLobbyPlayerStateComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, LobbyLoadout);
	DOREPLIFETIME(ThisClass, ReadyState);
}

void ULyraLobbyPlayerStateComponent::CopyProperties(UPlayerStateComponent* TargetPlayerStateComponent)
{
	Super::CopyProperties(TargetPlayerStateComponent);

	if (ULyraLobbyPlayerStateComponent* TargetLobbyComponent = Cast<ULyraLobbyPlayerStateComponent>(TargetPlayerStateComponent))
	{
		TargetLobbyComponent->LobbyLoadout = LobbyLoadout;
		TargetLobbyComponent->ReadyState = ReadyState;
	}
}

void ULyraLobbyPlayerStateComponent::Reset()
{
	Super::Reset();

	LobbyLoadout = FLyraLobbyPlayerLoadout();
	ReadyState = ELyraLobbyReadyState::Editing;
}

void ULyraLobbyPlayerStateComponent::ServerSubmitLobbyLoadout_Implementation(FLyraLobbyPlayerLoadout NewLoadout)
{
	ApplySubmittedLoadout(NewLoadout);
}

void ULyraLobbyPlayerStateComponent::ServerSetLobbyReady_Implementation(bool bReady)
{
	ApplyReadyState(bReady ? ELyraLobbyReadyState::Ready : ELyraLobbyReadyState::Editing);
}

void ULyraLobbyPlayerStateComponent::ApplySubmittedLoadout(const FLyraLobbyPlayerLoadout& NewLoadout)
{
	if (ReadyState == ELyraLobbyReadyState::Locked)
	{
		UE_LOG(LogLyra, Warning, TEXT("Lobby loadout rejected because loadout is locked. Owner=%s IncomingRevision=%d Slots=%d"),
			*GetNameSafe(GetOwner()),
			NewLoadout.Revision,
			NewLoadout.AccessorySlots.Num());
		return;
	}

	const int32 NextRevision = LobbyLoadout.Revision + 1;
	LobbyLoadout = NewLoadout;
	LobbyLoadout.Revision = NextRevision;
	OnRep_LobbyLoadout();

	if (AActor* Owner = GetOwner())
	{
		Owner->ForceNetUpdate();
	}

	UE_LOG(LogLyra, Log, TEXT("Lobby loadout applied. Owner=%s Revision=%d Slots=%d"),
		*GetNameSafe(GetOwner()),
		LobbyLoadout.Revision,
		LobbyLoadout.AccessorySlots.Num());

	if (ReadyState == ELyraLobbyReadyState::Ready)
	{
		ApplyReadyState(ELyraLobbyReadyState::Editing);
	}
}

void ULyraLobbyPlayerStateComponent::ApplyReadyState(ELyraLobbyReadyState NewReadyState)
{
	if (ReadyState == ELyraLobbyReadyState::Locked && NewReadyState != ELyraLobbyReadyState::Editing)
	{
		return;
	}

	if (ReadyState != NewReadyState)
	{
		ReadyState = NewReadyState;
		OnRep_ReadyState();
	}
}

bool ULyraLobbyPlayerStateComponent::HasAuthorityOwner() const
{
	const AActor* Owner = GetOwner();
	return Owner && Owner->HasAuthority();
}

void ULyraLobbyPlayerStateComponent::OnRep_LobbyLoadout()
{
	OnLobbyLoadoutChanged.Broadcast(LobbyLoadout);
}

void ULyraLobbyPlayerStateComponent::OnRep_ReadyState()
{
	OnReadyStateChanged.Broadcast(ReadyState);
}
