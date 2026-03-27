#include "BossCharacterBaseAiController.h"

// TODO: 필요한 헤더 include 추가
// 힌트: 거리 계산에 필요한 헤더는 CoreMinimal에 포함되어 있음


// ============================================================
// FBossAggroEntry
// ============================================================

bool FBossAggroEntry::operator<(const FBossAggroEntry& Other) const
{
	// TODO: 최대힙 조건 구현
	// 힌트: 내 TotalDamage가 Other보다 작으면 true
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
	// TODO: 유효성 검사 (nullptr, 중복 체크)

	// TODO: AllTargets에 추가

	// TODO: AggroHeap에 초기 엔트리(데미지 0) HeapPush
	// 힌트: AggroHeap.HeapPush(Entry)
}

void ABossCharacterBaseAiController::UpdateAggro(AActor* DamageDealer, float Damage)
{
	// TODO: 유효성 검사

	// TODO: AggroHeap에서 DamageDealer와 일치하는 엔트리 찾기 (선형 탐색)

	// TODO: 해당 엔트리의 TotalDamage += Damage

	// TODO: 힙 재정렬
	// 힌트: AggroHeap.Heapify()
}

AActor* ABossCharacterBaseAiController::GetHighestAggroTarget() const
{
	// TODO: AggroHeap이 비어있으면 nullptr 반환

	// TODO: AggroHeap.HeapTop()으로 최상위 엔트리의 액터 반환

	return nullptr;
}

AActor* ABossCharacterBaseAiController::GetNearestTarget() const
{
	if (!GetWorld()) return nullptr;
	APlayerController* PC= GetWorld()->GetFirstPlayerController();
	return PC->GetPawn();
}
