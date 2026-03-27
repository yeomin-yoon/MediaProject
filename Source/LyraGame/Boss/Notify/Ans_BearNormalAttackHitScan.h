#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Ans_BearNormalAttackHitScan.generated.h"

class UGameplayEffect;

UCLASS()
class LYRAGAME_API UAns_BearNormalAttackHitScan : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		float TotalDuration) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		float FrameDeltaTime) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;

private:
	// 트레이스 반경 (에디터에서 조절 가능)
	UPROPERTY(EditAnywhere, Category = "HitScan")
	float TraceRadius = 40.f;

	// 에디터에서 적용할 GE 클래스 지정 (GE_Boss_Damage_Normal)
	UPROPERTY(EditAnywhere, Category = "HitScan")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	// 같은 액터를 한 구간에서 여러 번 히트하지 않도록 저장
	TSet<TObjectPtr<AActor>> HitActors;
};
