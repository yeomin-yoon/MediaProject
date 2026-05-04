// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/GameStateComponent.h"
#include "Lobby/LyraLobbyTypes.h"

#include "LyraLobbyStateComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLyraLobbyPhaseChangedDelegate, ELyraLobbyPhase, LobbyPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLyraExpeditionConfigChangedDelegate, const FLyraExpeditionSessionConfig&, ExpeditionConfig);

UCLASS(BlueprintType, meta = (BlueprintSpawnableComponent))
class LYRAGAME_API ULyraLobbyStateComponent : public UGameStateComponent
{
	GENERATED_BODY()

public:
	ULyraLobbyStateComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Lyra|Lobby")
	void SetLobbyPhase(ELyraLobbyPhase NewPhase);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Lyra|Lobby")
	void ConfigureExpedition(const FLyraExpeditionSessionConfig& NewConfig);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Lyra|Lobby")
	int32 LockReadyPlayers();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Lyra|Lobby")
	void ResetLobby();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lyra|Lobby")
	ELyraLobbyPhase GetLobbyPhase() const { return LobbyPhase; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lyra|Lobby")
	const FLyraExpeditionSessionConfig& GetExpeditionConfig() const { return ExpeditionConfig; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lyra|Lobby")
	int32 GetReadyPlayerCount() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lyra|Lobby")
	int32 GetLockedPlayerCount() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lyra|Lobby")
	bool AreAllPlayersReady() const;

	UPROPERTY(BlueprintAssignable, Category = "Lyra|Lobby")
	FLyraLobbyPhaseChangedDelegate OnLobbyPhaseChanged;

	UPROPERTY(BlueprintAssignable, Category = "Lyra|Lobby")
	FLyraExpeditionConfigChangedDelegate OnExpeditionConfigChanged;

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UFUNCTION()
	void OnRep_LobbyPhase();

	UFUNCTION()
	void OnRep_ExpeditionConfig();

private:
	UPROPERTY(ReplicatedUsing = OnRep_LobbyPhase)
	ELyraLobbyPhase LobbyPhase = ELyraLobbyPhase::Open;

	UPROPERTY(ReplicatedUsing = OnRep_ExpeditionConfig)
	FLyraExpeditionSessionConfig ExpeditionConfig;
};
