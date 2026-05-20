#include "InventoryFragment_EquipEffect.h"
#include "Inventory/LyraInventoryItemInstance.h"

float UInventoryFragment_EquipEffect::RollRandomValue(
	const ULyraInventoryItemInstance* Instance) const
{
	if (!Instance)
		return MinValue;

	FRandomStream Stream(Instance->RandomSeed);

	return Stream.FRandRange(MinValue, MaxValue);
}

EItemOptionType
UInventoryFragment_EquipEffect::RollRandomOptionType(
	const ULyraInventoryItemInstance* Instance) const
{
	if (!Instance)
		return EItemOptionType::Attack;

	FRandomStream Stream(Instance->RandomSeed + 999);

	int32 Value = Stream.RandRange(0, 2);

	switch (Value)
	{
	case 0:
		return EItemOptionType::Attack;

	case 1:
		return EItemOptionType::Health;

	default:
		return EItemOptionType::Stamina;
	}
}

EItemRarity UInventoryFragment_EquipEffect::EvaluateRarity(
	float Value) const
{
	float Ratio =
		(Value - MinValue)
		/ (MaxValue - MinValue);

	if (Ratio >= 0.9f)
	{
		return EItemRarity::Epic;
	}

	if (Ratio >= 0.7f)
	{
		return EItemRarity::Rare;
	}

	if (Ratio >= 0.4f)
	{
		return EItemRarity::Uncommon;
	}

	return EItemRarity::Common;
}

float UInventoryFragment_EquipEffect::RollRandomValueFromSeed(
	int32 Seed) const
{
	FRandomStream Stream(Seed);

	return Stream.FRandRange(
		MinValue,
		MaxValue
	);
}