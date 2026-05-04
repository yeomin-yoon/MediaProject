// Copyright Epic Games, Inc. All Rights Reserved.

#include "Expedition/ExpeditionMonsterSpawnPoint.h"

#include "Components/SceneComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ExpeditionMonsterSpawnPoint)

AExpeditionMonsterSpawnPoint::AExpeditionMonsterSpawnPoint(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(SceneRoot);
}

FTransform AExpeditionMonsterSpawnPoint::MakeSpawnTransform() const
{
	FTransform SpawnTransform = GetActorTransform();

	if (RandomSpawnRadius > 0.0f)
	{
		const FVector2D RandomOffset2D = FMath::RandPointInCircle(RandomSpawnRadius);
		SpawnTransform.AddToTranslation(FVector(RandomOffset2D.X, RandomOffset2D.Y, 0.0f));
	}

	return SpawnTransform;
}

bool AExpeditionMonsterSpawnPoint::IsReadyForPeriodicSpawn(float WorldTimeSeconds) const
{
	return bAllowPeriodicSpawn && (WorldTimeSeconds >= NextAllowedPeriodicSpawnTimeSeconds);
}

void AExpeditionMonsterSpawnPoint::MarkPeriodicSpawnUsed(float WorldTimeSeconds)
{
	NextAllowedPeriodicSpawnTimeSeconds = WorldTimeSeconds + PeriodicSpawnCooldownSeconds;
}
