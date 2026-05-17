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