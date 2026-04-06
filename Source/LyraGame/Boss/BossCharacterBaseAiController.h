#pragma once

#include "CoreMinimal.h"
#include "ModularAIController.h"
#include "Teams/LyraTeamAgentInterface.h"
#include "GameplayStateTreeModule/Public/Components/StateTreeAIComponent.h"
#include "BossCharacterBaseAiController.generated.h"

namespace ETeamAttitude { enum Type : int; }

/**
 * 어그로 데이터 구조체
 * - 힙(최대힙) 정렬 기준: 누적 데미지가 높을수록 우선순위 높음
 * - UE의 TArray는 HeapPush/HeapPop/HeapTop 내장 → 별도 자료구조 불필요
 */
USTRUCT()
struct FBossAggroEntry
{
	GENERATED_BODY()

	TObjectPtr<AActor> TargetActor;
	float Damage = 0.f;

	// TODO: 최대힙 조건 구현 (Damage 낮은 쪽이 true 반환)
	bool operator<(const FBossAggroEntry& Other) const;
};

/**
 * 보스 AI 컨트롤러
 * - 어그로 시스템 (데미지 기반 힙) 관리
 * - StateTree에 타겟 정보 제공
 */
UCLASS()
class LYRAGAME_API ABossCharacterBaseAiController : public AModularAIController, public ILyraTeamAgentInterface
{
	GENERATED_BODY()

public:
	ABossCharacterBaseAiController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~ILyraTeamAgentInterface
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
	virtual FGenericTeamId GetGenericTeamId() const override;
	virtual FOnLyraTeamIndexChangedDelegate* GetOnTeamIndexChangedDelegate() override;
	//~End ILyraTeamAgentInterface

	void RegisterTarget(AActor* NewTarget);

	// TODO: DamageDealer의 누적 데미지 갱신 후 힙 재정렬
	void UpdateAggro(AActor* DamageDealer, float Damage);

	// TODO: AggroHeap이 비어있으면 nullptr, 아니면 HeapTop 액터 반환
	AActor* GetHighestAggroTarget() const;

	AActor* GetNearestTarget() const;

protected:
	virtual void OnPossess(APawn* InPawn) override;

private:
	UPROPERTY()
	TObjectPtr<UStateTreeAIComponent> StateTreeComp;

	TArray<TObjectPtr<AActor>> Targets;
	TArray<FBossAggroEntry> BossAggroEnters;

	UPROPERTY()
	FOnLyraTeamIndexChangedDelegate OnTeamChangedDelegate;

	// 보스는 항상 2번 팀 (플레이어=1번과 다른 팀 → CanCauseDamage=true)
	FGenericTeamId MyTeamID = FGenericTeamId(2);
};
