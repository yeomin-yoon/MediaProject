#pragma once

#include "CoreMinimal.h"
#include "Minion/MinionCharacterBase.h"
#include "DogMinionBase.generated.h"

UCLASS()
class LYRAGAME_API ADogMinionBase : public AMinionCharacterBase
{
	GENERATED_BODY()

public:
	ADogMinionBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};
