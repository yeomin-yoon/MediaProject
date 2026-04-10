#include "Boss/BossCharacterBase.h"
#include "Boss/BossCharacterBaseAiController.h"
#include "AbilitySystem/LyraAbilitySet.h"
#include "AbilitySystem/LyraAbilitySystemComponent.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/PlayerController.h"

ABossCharacterBase::ABossCharacterBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AIControllerClass = ABossCharacterBaseAiController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void ABossCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority() || !BossAbilitySet)
	{
		return;
	}

	ULyraAbilitySystemComponent* BossASC = Cast<ULyraAbilitySystemComponent>(GetAbilitySystemComponent());
	if (!BossASC)
	{
		return;
	}

	BossAbilitySet->GiveToAbilitySystem(BossASC, nullptr);
}

void ABossCharacterBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC)
	{
		return;
	}

	if (PC->WasInputKeyJustPressed(EKeys::One))
	{
		Test();
	}
}

void ABossCharacterBase::Test()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC || !TestAbilityClass)
	{
		return;
	}

	ASC->TryActivateAbilityByClass(TestAbilityClass);
}
