#pragma once

#include "CoreMinimal.h"
#include "Character/LyraCharacterWithAbilities.h"
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

protected:
	virtual void BeginPlay() override;

	// 에디터(BP_TestBoss)에서 지정할 어빌리티 셋 에셋
	// GiveToAbilitySystem() 호출 시 GA/GE/AttributeSet 일괄 부여
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Abilities")
	TObjectPtr<ULyraAbilitySet> BossAbilitySet;
};