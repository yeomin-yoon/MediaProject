#pragma once

#include "CoreMinimal.h"
#include "Inventory/LyraInventoryItemDefinition.h"
#include "InventoryFragment_EquipEffect.generated.h"

class UGameplayEffect;
class ULyraInventoryItemInstance;

UENUM(BlueprintType)
enum class EItemOptionType : uint8
{
	Attack,
	Health,
	Stamina
};

UCLASS()
class LYRAGAME_API UInventoryFragment_EquipEffect
	: public ULyraInventoryItemFragment
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere)
	TSubclassOf<UGameplayEffect> EquipEffect;

	UPROPERTY(EditAnywhere)
	float MinValue = 10.f;

	UPROPERTY(EditAnywhere)
	float MaxValue = 20.f;

	float RollRandomValue(
		const ULyraInventoryItemInstance* Instance) const;

	EItemOptionType RollRandomOptionType(
		const ULyraInventoryItemInstance* Instance) const;
};