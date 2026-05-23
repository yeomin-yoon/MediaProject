#include "Boss/BossCharacterBase.h"
#include "ActionCombatRuntimeTags.h"
#include "Boss/BossCharacterBaseAiController.h"
#include "AbilitySystem/LyraAbilitySet.h"
#include "AbilitySystem/LyraAbilitySystemComponent.h"
#include "AbilitySystemComponent.h"
#include "Character/LyraHealthComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
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

	ApplyBossNoPushSettings();

	if (!HasAuthority())
	{
		return;
	}

	ULyraAbilitySystemComponent* BossASC = Cast<ULyraAbilitySystemComponent>(GetAbilitySystemComponent());
	if (!BossASC)
	{
		return;
	}

	if (BossAbilitySet)
	{
		BossAbilitySet->GiveToAbilitySystem(BossASC, nullptr);
	}

	if (bIgnoreActionCombatReactions)
	{
		BossASC->SetLooseGameplayTagCount(ActionCombatRuntimeTags::Combat_State_Reaction_Immune, 1);
	}

	if (ULyraHealthComponent* HealthComp = ULyraHealthComponent::FindHealthComponent(this))
	{
		HealthComp->InitializeWithAbilitySystem(BossASC);
	}
}

void ABossCharacterBase::ApplyBossNoPushSettings()
{
	if (bPreventPawnPush)
	{
		if (UCapsuleComponent* Capsule = GetCapsuleComponent())
		{
			Capsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
			Capsule->CanCharacterStepUpOn = ECB_No;
		}
	}

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->bEnablePhysicsInteraction = false;
		MoveComp->RepulsionForce = 0.0f;
		MoveComp->MaxDepenetrationWithPawn = 0.0f;
		MoveComp->MaxDepenetrationWithPawnAsProxy = 0.0f;
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

