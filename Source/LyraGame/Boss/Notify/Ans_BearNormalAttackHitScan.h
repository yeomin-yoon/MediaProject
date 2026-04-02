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
	
	UPROPERTY(EditAnywhere, Category = "HitScan")
	float TraceRadius = 40.f;

	
	UPROPERTY(EditAnywhere, Category = "HitScan")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	
	TSet<TObjectPtr<AActor>> HitActors;
};
