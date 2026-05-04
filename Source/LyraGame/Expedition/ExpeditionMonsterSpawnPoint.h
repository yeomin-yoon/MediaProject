// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "ExpeditionMonsterSpawnPoint.generated.h"

UCLASS(BlueprintType, Blueprintable)
class LYRAGAME_API AExpeditionMonsterSpawnPoint : public AActor
{
	GENERATED_BODY()

public:
	AExpeditionMonsterSpawnPoint(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Expedition|Monster Spawn")
	FTransform MakeSpawnTransform() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Expedition|Monster Spawn")
	bool IsReadyForPeriodicSpawn(float WorldTimeSeconds) const;

	UFUNCTION(BlueprintCallable, Category = "Expedition|Monster Spawn")
	void MarkPeriodicSpawnUsed(float WorldTimeSeconds);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Expedition|Monster Spawn")
	TSoftClassPtr<AActor> MonsterClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Expedition|Monster Spawn")
	FName SpawnGroup = TEXT("Default");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Expedition|Monster Spawn")
	bool bSpawnOnInitialWave = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Expedition|Monster Spawn", meta = (ClampMin = "0"))
	int32 InitialSpawnCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Expedition|Monster Spawn", meta = (ClampMin = "0"))
	int32 PoolSize = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Expedition|Monster Spawn")
	bool bAllowPeriodicSpawn = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Expedition|Monster Spawn", meta = (ClampMin = "0.0"))
	float PeriodicSpawnCooldownSeconds = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Expedition|Monster Spawn", meta = (ClampMin = "0.0"))
	float RandomSpawnRadius = 0.0f;

private:
	float NextAllowedPeriodicSpawnTimeSeconds = 0.0f;
};
