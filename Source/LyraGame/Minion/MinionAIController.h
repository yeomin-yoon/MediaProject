#pragma once

#include "CoreMinimal.h"
#include "ModularAIController.h"
#include "Teams/LyraTeamAgentInterface.h"
#include "GameplayStateTreeModule/Public/Components/StateTreeAIComponent.h"
#include "MinionAIController.generated.h"

namespace ETeamAttitude { enum Type : int; }

UCLASS()
class LYRAGAME_API AMinionAIController : public AModularAIController, public ILyraTeamAgentInterface
{
	GENERATED_BODY()

public:
	AMinionAIController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~ILyraTeamAgentInterface
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
	virtual FGenericTeamId GetGenericTeamId() const override;
	virtual FOnLyraTeamIndexChangedDelegate* GetOnTeamIndexChangedDelegate() override;
	//~End ILyraTeamAgentInterface

	AActor* GetNearestTarget() const;

protected:
	virtual void OnPossess(APawn* InPawn) override;

private:
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UStateTreeAIComponent> StateTreeComp;

	UPROPERTY()
	FOnLyraTeamIndexChangedDelegate OnTeamChangedDelegate;

	// 미니언은 보스와 같은 팀 (플레이어=1번과 다른 팀)
	FGenericTeamId MyTeamID = FGenericTeamId(2);
};
