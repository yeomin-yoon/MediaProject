#include "BossCharacterBaseAiController.h"

bool FBossAggroEntry::operator<(const FBossAggroEntry& Other) const
{
	return false;
}

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
	if (NewTarget == nullptr) return;

	Targets.Add(NewTarget);

	FBossAggroEntry Entry;
	Entry.TargetActor = NewTarget;
	Entry.Damage = 0.f;
	BossAggroEnters.HeapPush(Entry);
}

void ABossCharacterBaseAiController::UpdateAggro(AActor* DamageDealer, float Damage)
{
}

AActor* ABossCharacterBaseAiController::GetHighestAggroTarget() const
{
	return nullptr;
}

AActor* ABossCharacterBaseAiController::GetNearestTarget() const
{
	if (!GetWorld())
	{
		UE_LOG(LogTemp, Warning, TEXT("[BossAI] GetNearestTarget: GetWorld() nullptr"));
		return nullptr;
	}
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BossAI] GetNearestTarget: PlayerController nullptr"));
		return nullptr;
	}
	APawn* PlayerPawn = PC->GetPawn();
	if (!PlayerPawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BossAI] GetNearestTarget: PlayerPawn nullptr"));
		return nullptr;
	}
	return PlayerPawn;
}

void ABossCharacterBaseAiController::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
}

FGenericTeamId ABossCharacterBaseAiController::GetGenericTeamId() const
{
	return MyTeamID;
}

FOnLyraTeamIndexChangedDelegate* ABossCharacterBaseAiController::GetOnTeamIndexChangedDelegate()
{
	return &OnTeamChangedDelegate;
}
