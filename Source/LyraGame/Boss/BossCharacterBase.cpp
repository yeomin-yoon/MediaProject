#include "Boss/BossCharacterBase.h"
#include "Boss/BossCharacterBaseAiController.h"
#include "AbilitySystem/LyraAbilitySet.h"
#include "AbilitySystem/LyraAbilitySystemComponent.h"
#include "AbilitySystemComponent.h"
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

	// 서버(Authority)에서만 GA를 부여해야 함 (GiveToAbilitySystem 내부도 체크하지만 명시적으로 확인)
	// 싱글플레이에서는 항상 Authority이므로 정상 동작
	if (!HasAuthority() || !BossAbilitySet)
	{
		return;
	}

	// LyraCharacterWithAbilities에 내장된 ASC를 가져옴
	// Cast 실패 시 nullptr → 부여 건너뜀
	ULyraAbilitySystemComponent* BossASC = Cast<ULyraAbilitySystemComponent>(GetAbilitySystemComponent());
	if (!BossASC)
	{
		return;
	}

	// BossAbilitySet에 등록된 GA/GE/AttributeSet을 ASC에 일괄 부여
	// OutGrantedHandles는 나중에 페이즈 전환 시 회수(TakeFromAbilitySystem)할 때 필요하면 멤버로 승격
	BossAbilitySet->GiveToAbilitySystem(BossASC, nullptr);
}

void ABossCharacterBase::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	
	bIsGrounded = true;
	bIsJumping = false;

	
	OnLandedDelegate.Broadcast();
}

bool ABossCharacterBase::GetIsGrounded()
{
	bIsGrounded = !GetCharacterMovement()->IsFalling();
	return bIsGrounded;
}

bool ABossCharacterBase::GetIsJumping()
{
	bIsJumping = GetCharacterMovement()->IsFalling();
	return bIsJumping;
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
	if (!ASC)
	{
		return;
	}

	if (!TestAbilityClass)
	{
		return;
	}

	ASC->TryActivateAbilityByClass(TestAbilityClass);
}

void ABossCharacterBase::SetIsGrounded(bool IsGrounded)
{
	bIsGrounded = IsGrounded;
}

void ABossCharacterBase::SetIsJumping(bool IsJumping)
{
	bIsJumping= IsJumping;
}


