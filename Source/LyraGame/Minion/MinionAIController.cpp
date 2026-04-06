#include "Minion/MinionAIController.h"

AMinionAIController::AMinionAIController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	StateTreeComp = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTreeAIComponent"));
}

void AMinionAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	StateTreeComp->StartLogic();
}

AActor* AMinionAIController::GetNearestTarget() const
{
	if (!GetWorld()) return nullptr;
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	return PC ? PC->GetPawn() : nullptr;
}

void AMinionAIController::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	// 미니언 팀 ID는 고정
}

FGenericTeamId AMinionAIController::GetGenericTeamId() const
{
	return MyTeamID;
}

FOnLyraTeamIndexChangedDelegate* AMinionAIController::GetOnTeamIndexChangedDelegate()
{
	return &OnTeamChangedDelegate;
}
