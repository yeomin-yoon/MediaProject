#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "BossAttackWeightData.generated.h"

class UGameplayAbility;

USTRUCT(BlueprintType)
struct FBossAttackEntry
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack")
	TSubclassOf<UGameplayAbility> AbilityClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack")
	FGameplayTag ResultTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weight", meta = (ClampMin = "0.0"))
	float BaseWeight = 10.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Distance", meta = (ClampMin = "0.0"))
	float MinDistance = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Distance", meta = (ClampMin = "0.0"))
	float MaxDistance = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weight", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float RecentUsePenalty = 0.3f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Phase")
	int32 ValidPhases = 3;
};

UCLASS(BlueprintType)
class LYRAGAME_API UBossAttackWeightData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attacks")
	TArray<FBossAttackEntry> Entries;
};
