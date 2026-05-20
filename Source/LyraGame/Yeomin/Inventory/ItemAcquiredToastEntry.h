#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ItemAcquiredToastEntry.generated.h"

class ULyraInventoryItemInstance;

UCLASS()
class LYRAGAME_API UItemAcquiredToastEntry : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TObjectPtr<UTexture2D> Icon;

	UPROPERTY()
	FText ItemText;

	UPROPERTY()
	TObjectPtr<ULyraInventoryItemInstance> ItemInstance;
	
	UPROPERTY()
	FLinearColor RarityColor;
};