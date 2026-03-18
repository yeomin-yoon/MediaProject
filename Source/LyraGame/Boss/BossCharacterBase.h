#pragma once

#include "CoreMinimal.h"
#include "GenericTeamAgentInterface.h"
#include "Character/LyraCharacterWithAbilities.h"
#include "BossCharacterBase.generated.h"

class ABossCharacterBaseAiController;
UCLASS()
class LYRAGAME_API ABossCharacterBase : public ALyraCharacterWithAbilities
{
	GENERATED_BODY()
public:
	ABossCharacterBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	UPROPERTY(BlueprintReadOnly, Category = "AI")
	TObjectPtr<ABossCharacterBaseAiController> BossAIController;

protected:
	virtual void BeginPlay() override;
	
	
};