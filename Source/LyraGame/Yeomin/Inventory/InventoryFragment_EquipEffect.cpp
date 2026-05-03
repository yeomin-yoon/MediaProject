#include "InventoryFragment_EquipEffect.h"
#include "Inventory/LyraInventoryItemInstance.h"

float UInventoryFragment_EquipEffect::RollRandomAttack(
	const ULyraInventoryItemInstance* Instance) const
{
	if (!Instance)
		return MinAttack;

	FRandomStream Stream(Instance->RandomSeed);

	float Range = MaxAttack - MinAttack;
	return MinAttack + Stream.FRand() * Range;
}