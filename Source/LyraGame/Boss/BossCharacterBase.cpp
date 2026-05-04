#include "Boss/BossCharacterBase.h"
#include "Boss/BossCharacterBaseAiController.h"
#include "AbilitySystem/LyraAbilitySet.h"
#include "AbilitySystem/LyraAbilitySystemComponent.h"
#include "AbilitySystemComponent.h"
#include "Character/LyraHealthComponent.h"
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

	if (ULyraHealthComponent* HealthComp = ULyraHealthComponent::FindHealthComponent(this))
	{
		HealthComp->InitializeWithAbilitySystem(BossASC);
	}
}

void ABossCharacterBase::DebugKill()
{
	ULyraHealthComponent* HealthComp = ULyraHealthComponent::FindHealthComponent(this);
	if (!HealthComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Death] DebugKill: HealthComponent 못 찾음"));
		return;
	}

	const float HealthBefore = HealthComp->GetHealth();
	const float MaxHealth = HealthComp->GetMaxHealth();
	const int32 DeathStateBefore = (int32)HealthComp->GetDeathState();

	UE_LOG(LogTemp, Warning,
		TEXT("[Death] DebugKill 호출 - Health=%.1f / Max=%.1f / DeathState=%d (0=NotDead) → DamageSelfDestruct"),
		HealthBefore, MaxHealth, DeathStateBefore);

	HealthComp->DamageSelfDestruct(true);

	const float HealthAfter = HealthComp->GetHealth();
	const int32 DeathStateAfter = (int32)HealthComp->GetDeathState();

	UE_LOG(LogTemp, Warning,
		TEXT("[Death] DamageSelfDestruct 직후 - Health=%.1f / DeathState=%d (1=DeathStarted, 2=DeathFinished)"),
		HealthAfter, DeathStateAfter);
}

void ABossCharacterBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC)
	{
		return;
	}

	
}

