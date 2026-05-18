#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ItemAcquiredToastEntry.generated.h"

class ULyraInventoryItemInstance;

UCLASS()
class UItemAcquiredToastEntry : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TObjectPtr<UTexture2D> Icon;

	UPROPERTY()
	FText ItemText;

	UPROPERTY()
	float DisplayDuration = 2.1f;

	UPROPERTY()
	TObjectPtr<ULyraInventoryItemInstance> ItemInstance;
};