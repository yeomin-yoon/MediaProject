#include "ItemDropComponent.h"

#include "InventoryFragment_EquipEffect.h"
#include "LyraWorldCollectable.h"
#include "Inventory/LyraInventoryItemDefinition.h"
#include "Engine/World.h"
#include "TimerManager.h"

UItemDropComponent::UItemDropComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
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

	ALyraWorldCollectable* Item =
		World->SpawnActor<ALyraWorldCollectable>(
			ItemClass,
			SpawnLocation,
			FRotator::ZeroRotator,
			SpawnParams
		);

	if (!Item)
		return;

	if (!Item->StaticInventory.Templates.IsValidIndex(0))
	{
		CurrentDropCount++;
		return;
	}

	// =========================
	// 1. Seed
	// =========================
	Item->RandomSeed = FMath::Rand();

	FRandomStream Stream(Item->RandomSeed);

	// =========================
	// 2. OptionType
	// =========================
	switch (Stream.RandRange(0, 2))
	{
	case 0: Item->OptionType = EItemOptionType::Attack; break;
	case 1: Item->OptionType = EItemOptionType::Health; break;
	case 2: Item->OptionType = EItemOptionType::Stamina; break;
	}

	// =========================
	// 3. Rarity
	// =========================
	const ULyraInventoryItemDefinition* DefCDO =
		GetDefault<ULyraInventoryItemDefinition>(
			Item->StaticInventory.Templates[0].ItemDef);

	if (DefCDO)
	{
		const UInventoryFragment_EquipEffect* Frag =
			Cast<UInventoryFragment_EquipEffect>(
				DefCDO->FindFragmentByClass(
					UInventoryFragment_EquipEffect::StaticClass()));

		if (Frag)
		{
			float Value = Frag->RollRandomValueFromSeed(Item->RandomSeed);
			Item->Rarity = Frag->EvaluateRarity(Value);
		}
	}

	// =========================
	// 4. PickupTemplate에도 그대로 고정
	// =========================
	Item->StaticInventory.Templates[0].RandomSeed = Item->RandomSeed;
	Item->StaticInventory.Templates[0].OptionType = Item->OptionType;
	Item->StaticInventory.Templates[0].Rarity = Item->Rarity;

	// =========================
	// 5. FX
	// =========================
	Item->ApplyNiagaraByOption();

	// =========================
	// 6. Launch
	// =========================
	float Angle = FMath::RandRange(0.f, 2 * PI);

	float Radius = FMath::RandRange(150.f, 300.f);
	float UpVelocity = FMath::RandRange(800.f, 950.f);

	FVector Velocity(
		FMath::Cos(Angle) * Radius,
		FMath::Sin(Angle) * Radius,
		UpVelocity
	);

	Item->LaunchItem(Velocity);

	CurrentDropCount++;
}