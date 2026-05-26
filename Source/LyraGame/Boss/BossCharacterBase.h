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
	
	//가중치 데이터
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Combat")
	TObjectPtr<UBossAttackWeightData> BossWeightData;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Combat")
	bool bPreventPawnPush = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Combat")
	bool bIgnoreActionCombatReactions = true;

	//현재 공격태그
	UPROPERTY(BlueprintReadOnly, Category = "Boss|Combat")
	FGameplayTag SelectedAttackTag;
	
	//가장 최근에 한 공격 태그
	UPROPERTY(BlueprintReadOnly, Category = "Boss|Combat")
	FGameplayTag LastAttackTag;
	
	
protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Abilities")
	TObjectPtr<ULyraAbilitySet> BossAbilitySet;

	UFUNCTION(BlueprintCallable, Category = "Boss|Debug")
	void DebugKill();

private:
	void ApplyBossNoPushSettings();
};
