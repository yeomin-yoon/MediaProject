#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "StateTreeExecutionTypes.h"
#include "GameplayTagContainer.h"
#include "STTask_Boss_BehaviorSelector.generated.h"

class AAIController;
class ABossCharacterBaseAiController;

USTRUCT(BlueprintType)
struct FBossActionData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere) FGameplayTag ActionTag;
	UPROPERTY(EditAnywhere) float MinRange = 0.f;
	UPROPERTY(EditAnywhere) float MaxRange = 500.f;
	UPROPERTY(EditAnywhere) float BaseWeight = 100.f;
	UPROPERTY(EditAnywhere) float CooldownDuration = 3.f;

	UPROPERTY(VisibleAnywhere) float LastUsedTime = -999.f;
	UPROPERTY(VisibleAnywhere) int32 ExecutionCount = 0;
	UPROPERTY(VisibleAnywhere) bool bWasLastAction = false;
};

UENUM()
enum class EBossSelectorState : uint8
{
	Idle,
	Moving,
	WaitingForGA,
};

USTRUCT()
struct FSTTask_Boss_BehaviorSelectorInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere) 
	TArray<FBossActionData> Actions; // 보스가 할 수 있는 행동들
	UPROPERTY() 
	EBossSelectorState SelectorState = EBossSelectorState::Idle; // 기본 스테이트
	UPROPERTY() 
	int32 ActiveActionIndex = -1;// 액션 고유 넘버
};

USTRUCT(meta = (DisplayName = "Boss Behavior Selector"))
struct FSTTask_Boss_BehaviorSelector : public FStateTreeTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FSTTask_Boss_BehaviorSelectorInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual bool Link(FStateTreeLinker& Linker) override;
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

private:
	int32 SelectAction(const TArray<FBossActionData>& Actions, float Distance, float CurrentTime) const;

	TStateTreeExternalDataHandle<AAIController> AIControllerHandle;
};
