#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "Inventory/LyraInventoryItemDefinition.h"
#include "InventoryFragment_EquipEffect.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;

UCLASS()
class LYRAGAME_API UInventoryFragment_EquipEffect : public ULyraInventoryItemFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	TSubclassOf<UGameplayEffect> EquipEffect;

	// 🔥 랜덤 범위
	UPROPERTY(EditAnywhere)
	float MinAttack = 10.f;

	UPROPERTY(EditAnywhere)
	float MaxAttack = 20.f;

	float RollRandomAttack(const ULyraInventoryItemInstance* Instance) const;
};