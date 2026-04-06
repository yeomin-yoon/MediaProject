#pragma once

#include "CoreMinimal.h"
#include "Character/LyraCharacterWithAbilities.h"
#include "Abilities/GameplayAbility.h"
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

	// 보스의 기본 공격 사정거리. MoveToRange Task / AutoTargeting Notify가 이 값을 공통 사용.
	// BP_TestBoss 등 각 보스 BP에서 값만 바꾸면 Task/Notify 코드 수정 없이 적용됨.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Combat")
	float AttackRange = 600.f;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	// 에디터(BP_TestBoss)에서 지정할 어빌리티 셋 에셋
	// GiveToAbilitySystem() 호출 시 GA/GE/AttributeSet 일괄 부여
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Abilities")
	TObjectPtr<ULyraAbilitySet> BossAbilitySet;
public:
	// 테스트용: 1번 키로 활성화할 GA 클래스 (BP_TestBoss에서 지정)
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Debug")
	TSubclassOf<UGameplayAbility> TestAbilityClass;

	// 테스트용: 1번 키 → TestAbilityClass 활성화
	UFUNCTION()
	void Test();
};