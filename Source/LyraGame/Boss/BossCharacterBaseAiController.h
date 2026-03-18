// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ModularAIController.h"
#include "Teams/LyraTeamAgentInterface.h"
#include "GameplayStateTreeModule/Public/Components/StateTreeAIComponent.h"
#include "BossCharacterBaseAiController.generated.h"

/**
 * 
 */
UCLASS()
class LYRAGAME_API ABossCharacterBaseAiController : public AModularAIController
{
	GENERATED_BODY()
public:
	ABossCharacterBaseAiController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
protected:
	
	UPROPERTY(EditDefaultsOnly, Category = "AI")
	 TObjectPtr<UStateTreeAIComponent> StateTreeComp;
	 virtual void OnPossess(APawn* InPawn) override;
};
