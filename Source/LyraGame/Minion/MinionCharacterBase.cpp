#include "Minion/MinionCharacterBase.h"
#include "Minion/MinionAIController.h"
#include "AbilitySystem/LyraAbilitySet.h"
#include "AbilitySystem/LyraAbilitySystemComponent.h"
#include "AbilitySystemComponent.h"
#include "Character/LyraHealthComponent.h"

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

	if (ULyraHealthComponent* HealthComp = ULyraHealthComponent::FindHealthComponent(this))
	{
		HealthComp->InitializeWithAbilitySystem(MinionASC);
	}
}
