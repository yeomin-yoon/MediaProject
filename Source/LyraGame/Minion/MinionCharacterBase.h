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

	// 에디터(BP_Minion 등)에서 지정할 어빌리티 셋
	UPROPERTY(EditDefaultsOnly, Category = "Minion|Abilities")
	TObjectPtr<ULyraAbilitySet> MinionAbilitySet;
};
