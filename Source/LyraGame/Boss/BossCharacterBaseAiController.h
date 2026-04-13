#pragma once

#include "CoreMinimal.h"
#include "ModularAIController.h"
#include "Teams/LyraTeamAgentInterface.h"
#include "GameplayStateTreeModule/Public/Components/StateTreeAIComponent.h"
#include "BossCharacterBaseAiController.generated.h"

namespace ETeamAttitude { enum Type : int; }

USTRUCT()
struct FBossAggroEntry
{
	GENERATED_BODY()

	TObjectPtr<AActor> TargetActor;
	float Damage = 0.f;

	bool operator<(const FBossAggroEntry& Other) const;
};

UCLASS()
class LYRAGAME_API ABossCharacterBaseAiController : public AModularAIController, public ILyraTeamAgentInterface
{
	GENERATED_BODY()

public:
	ABossCharacterBaseAiController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
	virtual FGenericTeamId GetGenericTeamId() const override;
	virtual FOnLyraTeamIndexChangedDelegate* GetOnTeamIndexChangedDelegate() override;

	void RegisterTarget(AActor* NewTarget);
	void UpdateAggro(AActor* DamageDealer, float Damage);
	AActor* GetHighestAggroTarget() const; //멀티시 데미지를 가장높게 입힌 타겟정보
	AActor* GetNearestTarget() const;// 가장 가까운 타겟 정보
	UStateTreeAIComponent* GetStateTreeComp() const { return StateTreeComp; }

protected:
	virtual void OnPossess(APawn* InPawn) override;

private:
	UPROPERTY()
	TObjectPtr<UStateTreeAIComponent> StateTreeComp;

	TArray<TObjectPtr<AActor>> Targets;
	TArray<FBossAggroEntry> BossAggroEnters;

	UPROPERTY()
	FOnLyraTeamIndexChangedDelegate OnTeamChangedDelegate;

	FGenericTeamId MyTeamID = FGenericTeamId(2); //플레이어가 팀 1 
};
