#include "Boss/GameplayCues/GC_Boss_MinionSpawnIn.h"
#include "NiagaraFunctionLibrary.h"

UGC_Boss_MinionSpawnIn::UGC_Boss_MinionSpawnIn()
{
}

bool UGC_Boss_MinionSpawnIn::OnExecute_Implementation(AActor* Target, const FGameplayCueParameters& Parameters) const
{
	bool bResult = Super::OnExecute_Implementation(Target, Parameters);

	if (!Target || !SpawnFX)
	{
		return bResult;
	}

	UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		Target->GetWorld(),
		SpawnFX,
		Parameters.Location,
		FRotator::ZeroRotator);

	return bResult;
}
