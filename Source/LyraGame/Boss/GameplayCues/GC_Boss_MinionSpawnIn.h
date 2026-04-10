#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Burst.h"
#include "NiagaraSystem.h"
#include "GC_Boss_MinionSpawnIn.generated.h"

UCLASS()
class LYRAGAME_API UGC_Boss_MinionSpawnIn : public UGameplayCueNotify_Burst
{
	GENERATED_BODY()

public:
	UGC_Boss_MinionSpawnIn();

protected:
	virtual bool OnExecute_Implementation(AActor* Target, const FGameplayCueParameters& Parameters) const override;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Cue")
	TObjectPtr<UNiagaraSystem> SpawnFX;
};
