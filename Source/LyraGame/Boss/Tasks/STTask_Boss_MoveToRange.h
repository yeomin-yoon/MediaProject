#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "StateTreeExecutionTypes.h"  // TStateTreeExternalDataHandle
#include "STTask_Boss_MoveToRange.generated.h"

class AAIController;
class ABossCharacterBaseAiController;

/**
 * 인스턴스 데이터
 * - 에디터에서 입력하는 파라미터만 담음
 * - AIController는 Schema에서 자동 제공 → Handle로 접근 (아래 Task 구조체 참고)
 */
USTRUCT()
struct FSTTask_Boss_MoveToRangeInstanceData
{
	GENERATED_BODY()
	// BossCharacterBase.AttackRange를 런타임에 읽음 → 여기 별도 값 없음
};

/**
 * Boss가 AcceptableRadius 안으로 이동하는 StateTree Task
 * - Schema(StateTreeAIComponentSchema)가 AIController를 컨텍스트로 제공
 * - Link()에서 Handle 등록 → 런타임에 GetExternalData()로 접근
 */
USTRUCT(meta = (DisplayName = "Boss Move To Range"))
struct FSTTask_Boss_MoveToRange : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FSTTask_Boss_MoveToRangeInstanceData;

	
	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	
	virtual bool Link(FStateTreeLinker& Linker) override;

	
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context,
	                                       const FStateTreeTransitionResult& Transition) const override;

	/**
	 * 매 프레임 호출
	 * TODO:
	 * 1. InstanceData, BossController 가져오기
	 * 2. GetNearestTarget() 재조회
	 * 3. 거리 계산 → AcceptableRadius 안이면 Succeeded
	 * 4. Running 반환
	 */
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context,
	                                 const float DeltaTime) const override;

	/**
	 * State 종료 시 호출
	 * TODO:
	 * 1. BossController 가져오기
	 * 2. StopMovement() 호출
	 */
	virtual void ExitState(FStateTreeExecutionContext& Context,
	                       const FStateTreeTransitionResult& Transition) const override;

private:
	// Schema(StateTreeAIComponentSchema)가 제공하는 AIController 핸들
	// Link()에서 등록, 런타임에 Context.GetExternalDataPtr()로 접근
	TStateTreeExternalDataHandle<AAIController> AIControllerHandle;
};
