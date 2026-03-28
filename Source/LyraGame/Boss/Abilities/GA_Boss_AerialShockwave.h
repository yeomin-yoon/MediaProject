#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/LyraGameplayAbility.h"
#include "GA_Boss_AerialShockwave.generated.h"

// TODO: BossCharacterBase forward declaration 추가
// 이유: 헤더에서 직접 include하면 컴파일 의존성이 생겨 빌드가 느려짐.
//       실제 멤버 접근은 .cpp에서만 하므로 헤더엔 forward declaration으로 충분함.
class ABossCharacterBase;

UCLASS()
class LYRAGAME_API UGA_Boss_AerialShockwave : public ULyraGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Boss_AerialShockwave();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

private:
	// TODO: 착지 감지를 위한 델리게이트 바인딩 함수 선언
	// 이유: Landed()는 Character에 있는 함수라 GA에서 직접 감지할 수 없음.
	//       BossCharacterBase::Landed()에서 이 함수를 호출하거나,
	//       델리게이트를 통해 GA에 착지 이벤트를 전달해야 함.
	UFUNCTION()
	void OnBossLanded();

	// 위로 솟구치는 힘의 크기
	// 이유: BP에서 값을 조정할 수 있어야 보스마다 다른 점프 높이를 줄 수 있음.
	UPROPERTY(EditDefaultsOnly, Category = "Boss|AerialAttack")
	float LaunchPower = 1000.f;

	// 착지 충격 범위 반경
	UPROPERTY(EditDefaultsOnly, Category = "Boss|AerialAttack")
	float HitRadius = 500.f;

	FGameplayAbilitySpecHandle CacheHandle;
	const FGameplayAbilityActorInfo* CacheActorInfo = nullptr;
	FGameplayAbilityActivationInfo CacheActivationInfo;
	
	
	
	// TODO: 착지 충격에 적용할 GameplayEffect 클래스 참조 추가
	// 이유: GE는 에셋이라 C++에서 직접 생성하지 않고 BP에서 지정해야
	//       디자이너가 데미지 수치를 수정할 수 있음.
	// UPROPERTY(EditDefaultsOnly, Category = "Boss|AerialAttack")
	// TSubclassOf<UGameplayEffect> DamageEffect;
};
