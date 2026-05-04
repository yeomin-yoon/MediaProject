// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/StreamableManager.h"
#include "GameFramework/Actor.h"

#include "ExpeditionMonsterSpawnManager.generated.h"

class AAIController;
class AExpeditionMonsterSpawnPoint;
class ULyraExperienceDefinition;

USTRUCT()
struct FExpeditionPooledMonster
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<AExpeditionMonsterSpawnPoint> SpawnPoint = nullptr;

	UPROPERTY()
	TObjectPtr<AActor> MonsterActor = nullptr;

	UPROPERTY()
	TSubclassOf<AActor> MonsterClass = nullptr;

	UPROPERTY()
	bool bActive = false;
};

UCLASS(BlueprintType, Blueprintable)
class LYRAGAME_API AExpeditionMonsterSpawnManager : public AActor
{
	GENERATED_BODY()

public:
	AExpeditionMonsterSpawnManager(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Expedition|Monster Spawn")
	void StartSpawnFlow();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Expedition|Monster Spawn")
	bool ActivateMonsterFromPool(AExpeditionMonsterSpawnPoint* SpawnPoint, bool bMarkPeriodicCooldown);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Expedition|Monster Spawn")
	bool ReturnMonsterToPool(AActor* MonsterActor);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Expedition|Monster Spawn")
	int32 GetActiveMonsterCount() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Expedition|Monster Spawn")
	int32 GetPooledMonsterCount() const { return MonsterPool.Num(); }

protected:
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintNativeEvent, Category = "Expedition|Monster Spawn")
	void OnPooledMonsterCreated(AActor* MonsterActor, AExpeditionMonsterSpawnPoint* SpawnPoint);

	UFUNCTION(BlueprintNativeEvent, Category = "Expedition|Monster Spawn")
	void OnPooledMonsterActivated(AActor* MonsterActor, AExpeditionMonsterSpawnPoint* SpawnPoint);

	UFUNCTION(BlueprintNativeEvent, Category = "Expedition|Monster Spawn")
	void OnPooledMonsterReturned(AActor* MonsterActor, AExpeditionMonsterSpawnPoint* SpawnPoint);

private:
	void HandleExperienceLoaded(const ULyraExperienceDefinition* CurrentExperience);
	void BeginAssetPreload();
	void HandleAssetsPreloaded();
	void BuildPendingPoolSpawnList();
	void SpawnNextPendingPoolActor();
	void FinishPoolWarmup();
	void ActivateInitialWave();
	void RunPeriodicSpawnTick();
	void CollectSpawnPoints();
	void SetMonsterActorDormant(AActor* MonsterActor, bool bDormant);
	void StartMonsterAI(AActor* MonsterActor);
	void StopMonsterAI(AActor* MonsterActor);
	int32 FindInactivePoolIndexForPoint(const AExpeditionMonsterSpawnPoint* SpawnPoint) const;

	UFUNCTION()
	void HandlePooledMonsterDestroyed(AActor* DestroyedActor);

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Expedition|Monster Spawn")
	bool bAutoStart = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Expedition|Monster Spawn")
	bool bWaitForExperienceLoaded = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Expedition|Monster Spawn", meta = (ClampMin = "0.0"))
	float InitialSpawnDelaySeconds = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Expedition|Monster Spawn", meta = (ClampMin = "0.0"))
	float PeriodicSpawnIntervalSeconds = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Expedition|Monster Spawn", meta = (ClampMin = "0"))
	int32 MaxActiveMonsters = 10;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Expedition|Monster Spawn", meta = (ClampMin = "0.0"))
	float PoolSpawnIntervalSeconds = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Expedition|Monster Spawn")
	bool bForceSpawnedActorsReplicate = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Expedition|Monster Spawn")
	bool bStartPeriodicSpawningAfterInitialWave = true;

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<AExpeditionMonsterSpawnPoint>> SpawnPoints;

	UPROPERTY(Transient)
	TArray<FExpeditionPooledMonster> MonsterPool;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AExpeditionMonsterSpawnPoint>> PendingPoolSpawnPoints;

	TSharedPtr<FStreamableHandle> PreloadHandle;
	FTimerHandle PoolSpawnTimerHandle;
	FTimerHandle InitialSpawnTimerHandle;
	FTimerHandle PeriodicSpawnTimerHandle;
	bool bSpawnFlowStarted = false;
};
