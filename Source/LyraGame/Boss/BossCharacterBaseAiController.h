#pragma once

#include "CoreMinimal.h"
#include "ModularAIController.h"
#include "Teams/LyraTeamAgentInterface.h"
#include "GameplayStateTreeModule/Public/Components/StateTreeAIComponent.h"
#include "BossCharacterBaseAiController.generated.h"

/**
 * 어그로 데이터 구조체
 * - 힙(최대힙) 정렬 기준: 누적 데미지가 높을수록 우선순위 높음
 * - UE의 TArray는 HeapPush/HeapPop/HeapTop 내장 → 별도 자료구조 불필요
 */
USTRUCT()
struct FBossAggroEntry
{
	GENERATED_BODY()

	// TODO: 플레이어 액터 변수 선언 (TObjectPtr 사용)
	TObjectPtr<AActor> TargetActor;
	// TODO: 누적 데미지 변수 선언 (float, 기본값 0.f)
	float Damage=0.f; // 현재까지 받은 데미지
	/**
	 * 힙 정렬 연산자 (최대힙: 데미지 높은 순)
	 * TODO: Other보다 내 데미지가 낮으면 true 반환 (최대힙 조건)
	 */
	bool operator<(const FBossAggroEntry& Other) const;
};

/**
 * 보스 AI 컨트롤러
 * - 어그로 시스템 (데미지 기반 힙) 관리
 * - StateTree에 타겟 정보 제공
 */
UCLASS()
class LYRAGAME_API ABossCharacterBaseAiController : public AModularAIController
{
	GENERATED_BODY()

public:
	ABossCharacterBaseAiController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/**
	 * 플레이어를 타겟 목록에 등록
	 * TODO: AllTargets에 추가, AggroHeap에 초기 엔트리 추가
	 */
	void RegisterTarget(AActor* NewTarget);

	/**
	 * 플레이어가 보스에게 데미지를 입혔을 때 호출
	 * TODO:
	 * 1. AggroHeap에서 해당 액터의 엔트리 찾기
	 * 2. 누적 데미지 갱신
	 * 3. 힙 재정렬 (Heapify)
	 */
	void UpdateAggro(AActor* DamageDealer, float Damage);

	/**
	 * 현재 가장 많은 데미지를 입힌 플레이어 반환
	 * TODO: AggroHeap이 비어있으면 nullptr, 아니면 HeapTop의 액터 반환
	 */
	AActor* GetHighestAggroTarget() const;

	/**
	 * 현재 보스와 가장 가까운 플레이어 반환
	 * TODO: AllTargets 순회 → 보스 위치와의 거리 계산 → 최솟값 반환
	 */
	AActor* GetNearestTarget() const;

protected:
	virtual void OnPossess(APawn* InPawn) override;

private:
	UPROPERTY()
	TObjectPtr<UStateTreeAIComponent> StateTreeComp;

	// TODO: 전체 플레이어 목록 변수 선언 (TArray<TObjectPtr<AActor>>)

	// TODO: 어그로 힙 변수 선언 (TArray<FBossAggroEntry>)
};
