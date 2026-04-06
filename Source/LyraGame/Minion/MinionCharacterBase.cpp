#include "Minion/MinionCharacterBase.h"
#include "Minion/MinionAIController.h"
#include "AbilitySystem/LyraAbilitySet.h"
#include "AbilitySystem/LyraAbilitySystemComponent.h"

AMinionCharacterBase::AMinionCharacterBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AIControllerClass = AMinionAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void AMinionCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority() || !MinionAbilitySet)
	{
		return;
	}

	ULyraAbilitySystemComponent* MinionASC = Cast<ULyraAbilitySystemComponent>(GetAbilitySystemComponent());
	if (!MinionASC)
	{
		return;
	}

	MinionAbilitySet->GiveToAbilitySystem(MinionASC, nullptr);
}
