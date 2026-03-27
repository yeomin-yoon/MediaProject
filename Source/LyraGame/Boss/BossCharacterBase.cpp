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

	// TODO: bIsGrounded = true, bIsJumping = false 세팅
	// 이유: 착지했으므로 애님 그래프 변수를 즉시 갱신해야
	//       착지 애니메이션 스테이트로 전환됨.
	bIsGrounded = true;
	bIsJumping = false;

	// TODO: 델리게이트 브로드캐스트
	// 이유: 바인딩된 모든 함수(GA의 OnBossLanded 등)를 한 번에 호출.
	//       누가 바인딩했는지 여기서 알 필요 없음 — 느슨한 결합.
	//       IsBound() 체크는 선택사항이지만 바인딩이 없을 때 불필요한 호출을 막을 수 있음.
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
		UE_LOG(LogTemp, Warning, TEXT("[BossTest] Tick: PlayerController 없음"));
		return;
	}

	if (PC->WasInputKeyJustPressed(EKeys::One))
	{
		UE_LOG(LogTemp, Warning, TEXT("[BossTest] 1번 키 감지됨 → Test() 호출"));
		Test();
	}
}

void ABossCharacterBase::Test()
{
	UE_LOG(LogTemp, Warning, TEXT("[BossTest] Test() 진입"));

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		UE_LOG(LogTemp, Error, TEXT("[BossTest] ASC 없음"));
		return;
	}

	if (!TestAbilityClass)
	{
		UE_LOG(LogTemp, Error, TEXT("[BossTest] TestAbilityClass 미설정 — BP_TestBoss Details에서 지정할 것"));
		return;
	}

	bool bActivated = ASC->TryActivateAbilityByClass(TestAbilityClass);
	UE_LOG(LogTemp, Warning, TEXT("[BossTest] TryActivateAbilityByClass 결과: %s"), bActivated ? TEXT("성공") : TEXT("실패"));
}

void ABossCharacterBase::SetIsGrounded(bool IsGrounded)
{
	bIsGrounded = IsGrounded;
}

void ABossCharacterBase::SetIsJumping(bool IsJumping)
{
	bIsJumping= IsJumping;
}


