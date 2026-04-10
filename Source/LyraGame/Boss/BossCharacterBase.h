#pragma once

#include "CoreMinimal.h"
#include "Character/LyraCharacterWithAbilities.h"
#include "Abilities/GameplayAbility.h"
#include "Data/BossAttackWeightData.h"
#include "BossCharacterBase.generated.h"

class ABossCharacterBaseAiController;
class ULyraAbilitySet;
class ULyraAbilitySystemComponent;

UCLASS()
class LYRAGAME_API ABossCharacterBase : public ALyraCharacterWithAbilities
{
	GENERATED_BODY()
public:
	ABossCharacterBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UPROPERTY(BlueprintReadOnly, Category = "AI")
	TObjectPtr<ABossCharacterBaseAiController> BossAIController;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Combat")
	float AttackRange = 600.f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Combat")
	TObjectPtr<UBossAttackWeightData> BossWeightData;

	UPROPERTY(BlueprintReadOnly, Category = "Boss|Combat")
	FGameplayTag SelectedAttackTag;

	UPROPERTY(BlueprintReadOnly, Category = "Boss|Combat")
	FGameplayTag LastAttackTag;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Abilities")
	TObjectPtr<ULyraAbilitySet> BossAbilitySet;

public:
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Debug")
	TSubclassOf<UGameplayAbility> TestAbilityClass;

	UFUNCTION()
	void Test();
};
