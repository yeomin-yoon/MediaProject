// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ItemDropComponent.generated.h"

class UNiagaraSystem;
class ALyraWorldCollectable;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LYRAGAME_API UItemDropComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UItemDropComponent();

protected:
	virtual void BeginPlay() override;
	
	UFUNCTION(BlueprintCallable)
	void DropItems();

private:
	void SpawnOneItem();
	
	UPROPERTY(EditAnywhere)
	TSubclassOf<ALyraWorldCollectable> ItemClass;

	UPROPERTY(EditAnywhere)
	int32 DropCount = 7;

	UPROPERTY(EditAnywhere)
	float SpawnInterval = 0.05f;

	UPROPERTY(EditAnywhere)
	float SpawnRadius = 30.f;

	int32 CurrentDropCount = 0;

	FTimerHandle DropTimerHandle;
};