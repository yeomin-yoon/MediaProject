// Fill out your copyright notice in the Description page of Project Settings.


#include "BossCharacterBaseAiController.h"

ABossCharacterBaseAiController::ABossCharacterBaseAiController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	StateTreeComp = CreateDefaultSubobject<UStateTreeAIComponent>(
	TEXT("StateTreeAIComponent"));
}

void ABossCharacterBaseAiController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	StateTreeComp->StartLogic();
}
