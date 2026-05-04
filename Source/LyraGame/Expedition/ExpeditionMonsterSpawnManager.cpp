// Copyright Epic Games, Inc. All Rights Reserved.

#include "Expedition/ExpeditionMonsterSpawnManager.h"

#include "AIController.h"
#include "BrainComponent.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Expedition/ExpeditionMonsterSpawnPoint.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Pawn.h"
#include "GameModes/LyraExperienceManagerComponent.h"
#include "LyraLogChannels.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ExpeditionMonsterSpawnManager)

AExpeditionMonsterSpawnManager::AExpeditionMonsterSpawnManager(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;
}

void AExpeditionMonsterSpawnManager::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority() && bAutoStart)
	{
		StartSpawnFlow();
	}
}

void AExpeditionMonsterSpawnManager::StartSpawnFlow()
{
	if (!HasAuthority() || bSpawnFlowStarted)
	{
		return;
	}

	bSpawnFlowStarted = true;

	if (bWaitForExperienceLoaded)
	{
		if (AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr)
		{
			if (ULyraExperienceManagerComponent* ExperienceComponent = GameState->FindComponentByClass<ULyraExperienceManagerComponent>())
			{
				ExperienceComponent->CallOrRegister_OnExperienceLoaded(FOnLyraExperienceLoaded::FDelegate::CreateUObject(this, &ThisClass::HandleExperienceLoaded));
				return;
			}
		}
	}

	BeginAssetPreload();
}

void AExpeditionMonsterSpawnManager::HandleExperienceLoaded(const ULyraExperienceDefinition* CurrentExperience)
{
	BeginAssetPreload();
}

void AExpeditionMonsterSpawnManager::BeginAssetPreload()
{
	CollectSpawnPoints();

	TArray<FSoftObjectPath> AssetsToLoad;
	for (const AExpeditionMonsterSpawnPoint* SpawnPoint : SpawnPoints)
	{
		if (!SpawnPoint || SpawnPoint->PoolSize <= 0 || SpawnPoint->MonsterClass.IsNull())
		{
			continue;
		}

		AssetsToLoad.AddUnique(SpawnPoint->MonsterClass.ToSoftObjectPath());
	}

	if (AssetsToLoad.IsEmpty())
	{
		HandleAssetsPreloaded();
		return;
	}

	PreloadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
		AssetsToLoad,
		FStreamableDelegate::CreateUObject(this, &ThisClass::HandleAssetsPreloaded),
		FStreamableManager::AsyncLoadHighPriority,
		false,
		false,
		TEXT("ExpeditionMonsterSpawnPreload"));
}

void AExpeditionMonsterSpawnManager::HandleAssetsPreloaded()
{
	BuildPendingPoolSpawnList();

	if (PendingPoolSpawnPoints.IsEmpty())
	{
		FinishPoolWarmup();
		return;
	}

	SpawnNextPendingPoolActor();
}

void AExpeditionMonsterSpawnManager::CollectSpawnPoints()
{
	SpawnPoints.Reset();

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (TActorIterator<AExpeditionMonsterSpawnPoint> It(World); It; ++It)
	{
		if (AExpeditionMonsterSpawnPoint* SpawnPoint = *It)
		{
			SpawnPoints.Add(SpawnPoint);
		}
	}

	SpawnPoints.Sort([](const AExpeditionMonsterSpawnPoint& Left, const AExpeditionMonsterSpawnPoint& Right)
	{
		return Left.GetFName().LexicalLess(Right.GetFName());
	});
}

void AExpeditionMonsterSpawnManager::BuildPendingPoolSpawnList()
{
	PendingPoolSpawnPoints.Reset();

	for (AExpeditionMonsterSpawnPoint* SpawnPoint : SpawnPoints)
	{
		if (!SpawnPoint || SpawnPoint->PoolSize <= 0 || SpawnPoint->MonsterClass.IsNull())
		{
			continue;
		}

		for (int32 Index = 0; Index < SpawnPoint->PoolSize; ++Index)
		{
			PendingPoolSpawnPoints.Add(SpawnPoint);
		}
	}
}

void AExpeditionMonsterSpawnManager::SpawnNextPendingPoolActor()
{
	if (!HasAuthority())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	while (!PendingPoolSpawnPoints.IsEmpty())
	{
		AExpeditionMonsterSpawnPoint* SpawnPoint = PendingPoolSpawnPoints[0];
		PendingPoolSpawnPoints.RemoveAt(0, 1, EAllowShrinking::No);

		if (!SpawnPoint)
		{
			continue;
		}

		UClass* LoadedClass = SpawnPoint->MonsterClass.Get();
		if (!LoadedClass)
		{
			UE_LOG(LogLyra, Warning, TEXT("Monster pool skipped %s because MonsterClass was not loaded."), *GetPathNameSafe(SpawnPoint));
			continue;
		}

		TSubclassOf<AActor> MonsterClass = LoadedClass;
		if (!MonsterClass)
		{
			UE_LOG(LogLyra, Warning, TEXT("Monster pool skipped %s because MonsterClass is not an actor class."), *GetPathNameSafe(SpawnPoint));
			continue;
		}

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = this;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		AActor* MonsterActor = World->SpawnActor<AActor>(MonsterClass, SpawnPoint->MakeSpawnTransform(), SpawnParameters);
		if (!MonsterActor)
		{
			UE_LOG(LogLyra, Warning, TEXT("Monster pool failed to spawn class %s at %s."), *GetNameSafe(MonsterClass.Get()), *GetPathNameSafe(SpawnPoint));
			continue;
		}

		if (bForceSpawnedActorsReplicate)
		{
			MonsterActor->SetReplicates(true);
		}

		MonsterActor->OnDestroyed.AddDynamic(this, &ThisClass::HandlePooledMonsterDestroyed);
		SetMonsterActorDormant(MonsterActor, true);

		FExpeditionPooledMonster& PoolEntry = MonsterPool.AddDefaulted_GetRef();
		PoolEntry.SpawnPoint = SpawnPoint;
		PoolEntry.MonsterActor = MonsterActor;
		PoolEntry.MonsterClass = MonsterClass;
		PoolEntry.bActive = false;

		OnPooledMonsterCreated(MonsterActor, SpawnPoint);
		break;
	}

	if (PendingPoolSpawnPoints.IsEmpty())
	{
		FinishPoolWarmup();
		return;
	}

	World->GetTimerManager().SetTimer(
		PoolSpawnTimerHandle,
		this,
		&ThisClass::SpawnNextPendingPoolActor,
		FMath::Max(PoolSpawnIntervalSeconds, 0.001f),
		false);
}

void AExpeditionMonsterSpawnManager::FinishPoolWarmup()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (InitialSpawnDelaySeconds <= 0.0f)
	{
		ActivateInitialWave();
		return;
	}

	World->GetTimerManager().SetTimer(
		InitialSpawnTimerHandle,
		this,
		&ThisClass::ActivateInitialWave,
		InitialSpawnDelaySeconds,
		false);
}

void AExpeditionMonsterSpawnManager::ActivateInitialWave()
{
	if (!HasAuthority())
	{
		return;
	}

	for (AExpeditionMonsterSpawnPoint* SpawnPoint : SpawnPoints)
	{
		if (!SpawnPoint || !SpawnPoint->bSpawnOnInitialWave)
		{
			continue;
		}

		const int32 Count = FMath::Max(SpawnPoint->InitialSpawnCount, 0);
		for (int32 Index = 0; Index < Count; ++Index)
		{
			if (GetActiveMonsterCount() >= MaxActiveMonsters)
			{
				break;
			}

			ActivateMonsterFromPool(SpawnPoint, false);
		}
	}

	if (bStartPeriodicSpawningAfterInitialWave && PeriodicSpawnIntervalSeconds > 0.0f)
	{
		GetWorldTimerManager().SetTimer(
			PeriodicSpawnTimerHandle,
			this,
			&ThisClass::RunPeriodicSpawnTick,
			PeriodicSpawnIntervalSeconds,
			true);
	}
}

void AExpeditionMonsterSpawnManager::RunPeriodicSpawnTick()
{
	if (!HasAuthority() || GetActiveMonsterCount() >= MaxActiveMonsters)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float WorldTimeSeconds = World->GetTimeSeconds();
	for (AExpeditionMonsterSpawnPoint* SpawnPoint : SpawnPoints)
	{
		if (!SpawnPoint || !SpawnPoint->IsReadyForPeriodicSpawn(WorldTimeSeconds))
		{
			continue;
		}

		if (ActivateMonsterFromPool(SpawnPoint, true))
		{
			if (GetActiveMonsterCount() >= MaxActiveMonsters)
			{
				break;
			}
		}
	}
}

bool AExpeditionMonsterSpawnManager::ActivateMonsterFromPool(AExpeditionMonsterSpawnPoint* SpawnPoint, bool bMarkPeriodicCooldown)
{
	if (!HasAuthority() || !SpawnPoint)
	{
		return false;
	}

	const int32 PoolIndex = FindInactivePoolIndexForPoint(SpawnPoint);
	if (PoolIndex == INDEX_NONE)
	{
		UE_LOG(LogLyra, Verbose, TEXT("No inactive pooled monster available for %s."), *GetPathNameSafe(SpawnPoint));
		return false;
	}

	FExpeditionPooledMonster& PoolEntry = MonsterPool[PoolIndex];
	AActor* MonsterActor = PoolEntry.MonsterActor;
	if (!MonsterActor)
	{
		return false;
	}

	MonsterActor->SetActorTransform(SpawnPoint->MakeSpawnTransform(), false, nullptr, ETeleportType::TeleportPhysics);
	SetMonsterActorDormant(MonsterActor, false);
	PoolEntry.bActive = true;

	if (bMarkPeriodicCooldown)
	{
		SpawnPoint->MarkPeriodicSpawnUsed(GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f);
	}

	OnPooledMonsterActivated(MonsterActor, SpawnPoint);
	return true;
}

bool AExpeditionMonsterSpawnManager::ReturnMonsterToPool(AActor* MonsterActor)
{
	if (!HasAuthority() || !MonsterActor)
	{
		return false;
	}

	for (FExpeditionPooledMonster& PoolEntry : MonsterPool)
	{
		if (PoolEntry.MonsterActor == MonsterActor)
		{
			SetMonsterActorDormant(MonsterActor, true);
			PoolEntry.bActive = false;
			OnPooledMonsterReturned(MonsterActor, PoolEntry.SpawnPoint);
			return true;
		}
	}

	return false;
}

int32 AExpeditionMonsterSpawnManager::GetActiveMonsterCount() const
{
	int32 ActiveCount = 0;

	for (const FExpeditionPooledMonster& PoolEntry : MonsterPool)
	{
		if (PoolEntry.bActive && IsValid(PoolEntry.MonsterActor))
		{
			++ActiveCount;
		}
	}

	return ActiveCount;
}

void AExpeditionMonsterSpawnManager::SetMonsterActorDormant(AActor* MonsterActor, bool bDormant)
{
	if (!MonsterActor)
	{
		return;
	}

	MonsterActor->SetActorHiddenInGame(bDormant);
	MonsterActor->SetActorEnableCollision(!bDormant);
	MonsterActor->SetActorTickEnabled(!bDormant);

	if (bDormant)
	{
		StopMonsterAI(MonsterActor);
	}
	else
	{
		StartMonsterAI(MonsterActor);
		MonsterActor->ForceNetUpdate();
	}
}

void AExpeditionMonsterSpawnManager::StartMonsterAI(AActor* MonsterActor)
{
	APawn* Pawn = Cast<APawn>(MonsterActor);
	if (!Pawn)
	{
		return;
	}

	if (!Pawn->GetController())
	{
		Pawn->SpawnDefaultController();
	}

	if (AAIController* AIController = Cast<AAIController>(Pawn->GetController()))
	{
		if (UBrainComponent* BrainComponent = AIController->GetBrainComponent())
		{
			BrainComponent->RestartLogic();
		}

		AIController->SetActorTickEnabled(true);
	}
}

void AExpeditionMonsterSpawnManager::StopMonsterAI(AActor* MonsterActor)
{
	APawn* Pawn = Cast<APawn>(MonsterActor);
	if (!Pawn)
	{
		return;
	}

	if (AAIController* AIController = Cast<AAIController>(Pawn->GetController()))
	{
		if (UBrainComponent* BrainComponent = AIController->GetBrainComponent())
		{
			BrainComponent->StopLogic(TEXT("Expedition monster pooled"));
		}

		AIController->SetActorTickEnabled(false);
	}
}

int32 AExpeditionMonsterSpawnManager::FindInactivePoolIndexForPoint(const AExpeditionMonsterSpawnPoint* SpawnPoint) const
{
	for (int32 Index = 0; Index < MonsterPool.Num(); ++Index)
	{
		const FExpeditionPooledMonster& PoolEntry = MonsterPool[Index];
		if (!PoolEntry.bActive && IsValid(PoolEntry.MonsterActor) && PoolEntry.SpawnPoint == SpawnPoint)
		{
			return Index;
		}
	}

	return INDEX_NONE;
}

void AExpeditionMonsterSpawnManager::HandlePooledMonsterDestroyed(AActor* DestroyedActor)
{
	for (FExpeditionPooledMonster& PoolEntry : MonsterPool)
	{
		if (PoolEntry.MonsterActor == DestroyedActor)
		{
			PoolEntry.MonsterActor = nullptr;
			PoolEntry.bActive = false;
			return;
		}
	}
}

void AExpeditionMonsterSpawnManager::OnPooledMonsterCreated_Implementation(AActor* MonsterActor, AExpeditionMonsterSpawnPoint* SpawnPoint)
{
}

void AExpeditionMonsterSpawnManager::OnPooledMonsterActivated_Implementation(AActor* MonsterActor, AExpeditionMonsterSpawnPoint* SpawnPoint)
{
}

void AExpeditionMonsterSpawnManager::OnPooledMonsterReturned_Implementation(AActor* MonsterActor, AExpeditionMonsterSpawnPoint* SpawnPoint)
{
}
