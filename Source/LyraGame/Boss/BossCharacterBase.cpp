// Fill out your copyright notice in the Description page of Project Settings.


#include "Boss/BossCharacterBase.h"
#include "Boss/BossCharacterBaseAiController.h"

ABossCharacterBase::ABossCharacterBase(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    AIControllerClass = ABossCharacterBaseAiController::StaticClass();
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}
