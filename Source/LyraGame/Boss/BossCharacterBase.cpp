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

	//ASC에 등록 (Granted Effects 적용 → MaxHealth 등 어트리뷰트 초기화)
	BossAbilitySet->GiveToAbilitySystem(BossASC, nullptr);

	// HealthComponent를 ASC와 연결.
	// ALyraCharacterWithAbilities는 PawnExtComponent 흐름을 안 타서 OnAbilitySystemInitialized()
	// 콜백이 자동 호출되지 않음 → InitializeWithAbilitySystem를 직접 호출해야 HealthSet 포인터가 세팅됨.
	if (ULyraHealthComponent* HealthComp = ULyraHealthComponent::FindHealthComponent(this))
	{
		HealthComp->InitializeWithAbilitySystem(BossASC);
	}
	// HP 0 → OnOutOfHealth → ASC가 GameplayEvent.Death 송출 → GA_Boss_Death 자동 트리거
	// (GA_Boss_Death가 ULyraGameplayAbility_Death 상속이라 별도 OnDeathStarted 바인딩 불필요)
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

