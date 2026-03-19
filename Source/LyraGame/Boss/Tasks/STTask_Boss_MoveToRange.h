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

	// 공격 패턴마다 에디터에서 다르게 설정
	UPROPERTY(EditAnywhere)
	float AcceptableRadius = 200.f;
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

	// StateTree 런타임이 인스턴스 데이터 타입을 알 수 있도록 반환
	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	/**
	 * StateTree 에셋 로드 시 1회 호출 → 외부 데이터 핸들 등록
	 * TODO:
	 * 1. Linker.LinkExternalData(AIControllerHandle) 호출
	 * 2. return true
	 */
	virtual bool Link(FStateTreeLinker& Linker) override;

	/**
	 * State 진입 시 호출
	 * TODO:
	 * 1. InstanceData 가져오기: Context.GetInstanceData(*this)
	 * 2. AIController 가져오기: Context.GetExternalDataPtr(AIControllerHandle) → BossController로 Cast
	 * 3. BossController nullptr 체크 → Failed
	 * 4. BossController->GetNearestTarget() 으로 타겟 가져오기 → nullptr 체크
	 * 5. GetPawn() nullptr 체크
	 * 6. 거리 계산 → AcceptableRadius 안이면 Succeeded
	 * 7. MoveToActor() 호출 → Running
	 */
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
