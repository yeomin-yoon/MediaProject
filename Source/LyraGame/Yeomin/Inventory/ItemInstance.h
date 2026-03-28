// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ItemInstance.generated.h"

class UItemDefinition;
/**
 * 
 */
UCLASS()
class LYRAGAME_API UItemInstance : public UObject
{
	GENERATED_BODY()
	
public:
	UItemInstance();
	
protected:
	UPROPERTY()
	FGuid ItemGuid;
	
	UPROPERTY()
	TObjectPtr<UItemDefinition> ItemDef;
	
	UPROPERTY()
	int32 PosX;

	UPROPERTY()
	int32 PosY;
	
	UPROPERTY(EditAnywhere)
	bool bUseRandomStats;

	UPROPERTY(EditAnywhere)
	bool bGrantAbility;

	// UPROPERTY(EditAnywhere)
	// TArray<FGameplayTag> PossibleStats;
	//
	// UPROPERTY(EditAnywhere)
	// TArray<TSubclassOf<UGameplayAbility>> GrantedAbilities;
};
