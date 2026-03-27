#pragma once

#include "CoreMinimal.h"
#include "Character/LyraCharacterWithAbilities.h"
#include "Abilities/GameplayAbility.h"
#include "BossCharacterBase.generated.h"

class ABossCharacterBaseAiController;
class ULyraAbilitySet;
class ULyraAbilitySystemComponent;

// TODO: 델리게이트 타입 선언
// 이유: DECLARE_MULTICAST_DELEGATE는 C++끼리만 통신할 때 사용.
//       BP에서도 바인딩이 필요하면 DECLARE_DYNAMIC_MULTICAST_DELEGATE를 써야 하지만
//       GA는 C++ 이므로 더 가벼운 MULTICAST로 충분함.
//       클래스 밖 전역에 선언해야 다른 파일에서 타입을 재사용할 수 있음.
DECLARE_MULTICAST_DELEGATE(FOnBossLanded);

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

	// TODO: 착지 시 호출되는 ACharacter 가상 함수 오버라이드
	// 이유: GA에 착지 이벤트를 전달하는 진입점.
	//       물리 기반으로 실제 착지 순간에 호출되므로 타이머보다 정확함.
	virtual void Landed(const FHitResult& Hit) override;

	// 에디터(BP_TestBoss)에서 지정할 어빌리티 셋 에셋
	// GiveToAbilitySystem() 호출 시 GA/GE/AttributeSet 일괄 부여
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Abilities")
	TObjectPtr<ULyraAbilitySet> BossAbilitySet;
	public:
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Boss|Abilities") 
	bool bIsGrounded=true;
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Boss|Abilities") 
	bool bIsJumping=false;
public:
	bool GetIsGrounded();
	bool GetIsJumping();
	void SetIsGrounded(bool IsGrounded);
	void SetIsJumping(bool IsJumping);

	// TODO: 델리게이트 인스턴스 선언
	// 이유: GA가 여기에 바인딩하고, Landed()에서 Broadcast하면
	//       바인딩된 모든 함수가 자동으로 호출되는 구조.
	//       UPROPERTY 불필요 — UObject가 아니라 일반 C++ 구조체임.
	FOnBossLanded OnLandedDelegate;

	// 테스트용: 1번 키로 활성화할 GA 클래스 (BP_TestBoss에서 지정)
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Debug")
	TSubclassOf<UGameplayAbility> TestAbilityClass;

	// 테스트용: 1번 키 → TestAbilityClass 활성화
	UFUNCTION()
	void Test();
};