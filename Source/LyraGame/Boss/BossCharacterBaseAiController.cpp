#include "BossCharacterBaseAiController.h"

// ============================================================
// FBossAggroEntry
// ============================================================

bool FBossAggroEntry::operator<(const FBossAggroEntry& Other) const
{
	// TODO: 최대힙 조건 구현 (내 Damage가 Other보다 작으면 true)
	return false;
}


// ============================================================
// ABossCharacterBaseAiController
// ============================================================

ABossCharacterBaseAiController::ABossCharacterBaseAiController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	StateTreeComp = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTreeAIComponent"));
}

void ABossCharacterBaseAiController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	StateTreeComp->StartLogic();
}

void ABossCharacterBaseAiController::RegisterTarget(AActor* NewTarget)
{
	if (NewTarget == nullptr) return;

	Targets.Add(NewTarget);

	FBossAggroEntry Entry;
	Entry.TargetActor = NewTarget;
	Entry.Damage = 0.f;
	BossAggroEnters.HeapPush(Entry);
}

void ABossCharacterBaseAiController::UpdateAggro(AActor* DamageDealer, float Damage)
{
	// TODO: 유효성 검사
	// TODO: BossAggroEnters에서 DamageDealer 엔트리 찾기 (선형 탐색)
	// TODO: 해당 엔트리 Damage += Damage
	// TODO: BossAggroEnters.Heapify() 로 힙 재정렬
}

AActor* ABossCharacterBaseAiController::GetHighestAggroTarget() const
{
	// TODO: BossAggroEnters 비어있으면 nullptr
	// TODO: BossAggroEnters.HeapTop().TargetActor 반환
	return nullptr;
}

AActor* ABossCharacterBaseAiController::GetNearestTarget() const
{
	if (!GetWorld()) return nullptr;
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	return PC->GetPawn();
}

// ============================================================
// ILyraTeamAgentInterface
// ============================================================

void ABossCharacterBaseAiController::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	// 보스 팀 ID는 고정 (외부 변경 불필요)
}

FGenericTeamId ABossCharacterBaseAiController::GetGenericTeamId() const
{
	return MyTeamID;
}

FOnLyraTeamIndexChangedDelegate* ABossCharacterBaseAiController::GetOnTeamIndexChangedDelegate()
{
	return &OnTeamChangedDelegate;
}
