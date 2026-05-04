// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/PlayerStateComponent.h"
#include "Lobby/LyraLobbyTypes.h"

#include "LyraLobbyPlayerStateComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLyraLobbyLoadoutChangedDelegate, const FLyraLobbyPlayerLoadout&, LobbyLoadout);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLyraLobbyReadyStateChangedDelegate, ELyraLobbyReadyState, ReadyState);

UCLASS(BlueprintType, meta = (BlueprintSpawnableComponent))
class LYRAGAME_API ULyraLobbyPlayerStateComponent : public UPlayerStateComponent
{
	GENERATED_BODY()

public:
	ULyraLobbyPlayerStateComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, Category = "Lyra|Lobby")
	void SubmitLobbyLoadout(const FLyraLobbyPlayerLoadout& NewLoadout);

	UFUNCTION(BlueprintCallable, Category = "Lyra|Lobby")
	void SetLobbyReady(bool bReady);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Lyra|Lobby")
	void LockLobbyLoadout();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Lyra|Lobby")
	void UnlockLobbyLoadout();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lyra|Lobby")
	const FLyraLobbyPlayerLoadout& GetLobbyLoadout() const { return LobbyLoadout; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lyra|Lobby")
	ELyraLobbyReadyState GetReadyState() const { return ReadyState; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lyra|Lobby")
	bool IsReadyOrLocked() const;

	UPROPERTY(BlueprintAssignable, Category = "Lyra|Lobby")
	FLyraLobbyLoadoutChangedDelegate OnLobbyLoadoutChanged;

	UPROPERTY(BlueprintAssignable, Category = "Lyra|Lobby")
	FLyraLobbyReadyStateChangedDelegate OnReadyStateChanged;

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void CopyProperties(UPlayerStateComponent* TargetPlayerStateComponent) override;
	virtual void Reset() override;

private:
	UFUNCTION(Server, Reliable)
	void ServerSubmitLobbyLoadout(FLyraLobbyPlayerLoadout NewLoadout);

	UFUNCTION(Server, Reliable)
	void ServerSetLobbyReady(bool bReady);

	void ApplySubmittedLoadout(const FLyraLobbyPlayerLoadout& NewLoadout);
	void ApplyReadyState(ELyraLobbyReadyState NewReadyState);
	bool HasAuthorityOwner() const;

	UFUNCTION()
	void OnRep_LobbyLoadout();

	UFUNCTION()
	void OnRep_ReadyState();

private:
	UPROPERTY(ReplicatedUsing = OnRep_LobbyLoadout)
	FLyraLobbyPlayerLoadout LobbyLoadout;

	UPROPERTY(ReplicatedUsing = OnRep_ReadyState)
	ELyraLobbyReadyState ReadyState = ELyraLobbyReadyState::Editing;
};
