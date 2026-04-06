#include "ItemDropComponent.h"
#include "LyraWorldCollectable.h"
#include "Engine/World.h"
#include "TimerManager.h"

UItemDropComponent::UItemDropComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UItemDropComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UItemDropComponent::DropItems()
{
	if (!ItemClass) return;
	if (!GetWorld()) return;

	CurrentDropCount = 0;

	GetWorld()->GetTimerManager().SetTimer(
		DropTimerHandle,
		this,
		&UItemDropComponent::SpawnOneItem,
		SpawnInterval,
		true
	);
}

void UItemDropComponent::SpawnOneItem()
{
	if (CurrentDropCount >= DropCount)
	{
		GetWorld()->GetTimerManager().ClearTimer(DropTimerHandle);
		return;
	}

	UWorld* World = GetWorld();
	if (!World) return;

	FVector OwnerLocation = GetOwner()->GetActorLocation();

	FVector RandomOffset = FVector(
		FMath::RandRange(-SpawnRadius, SpawnRadius),
		FMath::RandRange(-SpawnRadius, SpawnRadius),
		20.f
	);

	FVector SpawnLocation = OwnerLocation + RandomOffset;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwner();
	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	ALyraWorldCollectable* Item = World->SpawnActor<ALyraWorldCollectable>(
		ItemClass,
		SpawnLocation,
		FRotator::ZeroRotator,
		SpawnParams
	);

	if (Item)
	{
		// ===== 분수 Velocity 계산 =====
		float Angle = FMath::RandRange(0.f, 2 * PI);
		float Radius = FMath::RandRange(150.f, 300.f);   // XY 퍼짐
		float UpVelocity = FMath::RandRange(800.f, 950.f); // 위로 쏘는 힘

		FVector Velocity = FVector(
			FMath::Cos(Angle) * Radius,
			FMath::Sin(Angle) * Radius,
			UpVelocity
		);

		Item->LaunchItem(Velocity);
	}

	CurrentDropCount++;
}