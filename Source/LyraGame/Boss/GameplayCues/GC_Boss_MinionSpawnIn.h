#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Burst.h"
#include "NiagaraSystem.h"
#include "GC_Boss_MinionSpawnIn.generated.h"

/**
 * 미니언 스폰 시 재생되는 GameplayCue (일회성 이펙트)
 * GameplayCueTags: GameplayCue.Boss.Minion.SpawnIn
 */
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
