#pragma once

#include "CoreMinimal.h"
#include "Character/LyraCharacterWithAbilities.h"
#include "MinionCharacterBase.generated.h"

class AMinionAIController;
class ULyraAbilitySet;

UCLASS()
class LYRAGAME_API AMinionCharacterBase : public ALyraCharacterWithAbilities
{
	GENERATED_BODY()

public:
	AMinionCharacterBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, Category = "Minion|Abilities")
	TObjectPtr<ULyraAbilitySet> MinionAbilitySet;
};
