#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "StateTreeExecutionTypes.h"
#include "STTask_Boss_NormalAttack.generated.h"

class AAIController;

/**
 * 인스턴스 데이터
 * - StateTree Task는 const 함수만 가지므로 상태는 여기에 보관
 * - bAbilityEnded: GA 완료 시 람다가 true로 설정 → Tick에서 감지
 * - AbilityEndedHandle: ExitState에서 델리게이트 제거용
 */
USTRUCT()
struct FSTTask_Boss_NormalAttackInstanceData
{
	GENERATED_BODY()

	// TODO: GA가 끝났는지 여부를 저장할 bool 변수 선언 (기본값 false)
	bool bIsEnd = false;
	// TODO: AbilityEndedCallbacks 바인딩 핸들 선언 (타입: FDelegateHandle)
	FDelegateHandle AbilityEndCallbackHandle;
};

/**
 * GA_Boss_NormalAttack을 발동하고 완료까지 기다리는 StateTree Task
 */
USTRUCT(meta = (DisplayName = "Boss Normal Attack"))
struct FSTTask_Boss_NormalAttack : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FSTTask_Boss_NormalAttackInstanceData;

	// TODO: GetInstanceDataType() 오버라이드 (MoveToRange 참고)
	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual bool Link(FStateTreeLinker& Linker) override;

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context,
		const FStateTreeTransitionResult& Transition) const override;

	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context,
		const float DeltaTime) const override;

	virtual void ExitState(FStateTreeExecutionContext& Context,
		const FStateTreeTransitionResult& Transition) const override;

private:
	// TODO: AIController 핸들 선언 (MoveToRange 참고)
	TStateTreeExternalDataHandle<AAIController> AIControllerHandle;
};
