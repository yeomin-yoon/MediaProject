// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameplayTagContainer.h"
#include "Engine/AssetManagerTypes.h"

#include "LyraLobbyTypes.generated.h"

UENUM(BlueprintType)
enum class ELyraLobbyReadyState : uint8
{
	Editing,
	Ready,
	Locked
};

UENUM(BlueprintType)
enum class ELyraLobbyPhase : uint8
{
	Open,
	Locking,
	Locked,
	Launching,
	InExpedition
};

USTRUCT(BlueprintType)
struct FLyraLobbyAccessorySelection
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lyra|Lobby")
	FGameplayTag SlotTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lyra|Lobby")
	FPrimaryAssetId AccessoryId;
};

USTRUCT(BlueprintType)
struct FLyraLobbyPlayerLoadout
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lyra|Lobby")
	FPrimaryAssetId CharacterPresetId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lyra|Lobby")
	FPrimaryAssetId DoctrinePresetId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lyra|Lobby")
	TArray<FPrimaryAssetId> EquipmentPartIds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lyra|Lobby")
	TArray<FLyraLobbyAccessorySelection> AccessorySlots;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lyra|Lobby")
	FPrimaryAssetId ActiveCombatStyleId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lyra|Lobby")
	int32 Revision = 0;
};

USTRUCT(BlueprintType)
struct FLyraExpeditionSessionConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lyra|Lobby")
	FPrimaryAssetId UserFacingExperienceId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lyra|Lobby")
	FPrimaryAssetId MapId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lyra|Lobby")
	FPrimaryAssetId RulesetId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lyra|Lobby")
	int32 Seed = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lyra|Lobby")
	int32 Revision = 0;
};
