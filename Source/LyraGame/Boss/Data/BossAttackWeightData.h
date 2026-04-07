#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "BossAttackWeightData.generated.h"

class UGameplayAbility;

// 공격 패턴 하나의 선택 조건 및 가중치 정보
USTRUCT(BlueprintType)
struct FBossAttackEntry
{
	GENERATED_BODY()

	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack")
	TSubclassOf<UGameplayAbility> AbilityClass;

	// 선택 결과로 저장할 태그 (StateTree Transition 분기용)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack")
	FGameplayTag ResultTag;

	// 기본 가중치 (높을수록 선택 확률 증가)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weight", meta = (ClampMin = "0.0"))
	float BaseWeight = 10.f;

	// 유효 거리 최솟값 (범위 밖이면 선택 제외)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Distance", meta = (ClampMin = "0.0"))
	float MinDistance = 0.f;

	// 유효 거리 최댓값 (0이면 무제한)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Distance", meta = (ClampMin = "0.0"))
	float MaxDistance = 0.f;

	// 직전에 이 패턴을 사용했을 때 가중치에 곱할 값 (0~1, 낮을수록 연속 선택 억제)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weight", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float RecentUsePenalty = 0.3f;

	// 유효한 페이즈 비트플래그 (1=Phase1, 2=Phase2, 3=둘다)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Phase")
	int32 ValidPhases = 3;
};

// 보스 공격 패턴 가중치 데이터. 보스마다 에셋을 따로 만들어 BossCharacterBase에 지정.
UCLASS(BlueprintType)
class LYRAGAME_API UBossAttackWeightData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attacks")
	TArray<FBossAttackEntry> Entries;
};
