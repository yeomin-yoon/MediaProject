// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AN_AutoTargeting.generated.h"

/**
 * 공격 몽타주 시작 시점에 보스가 플레이어 방향으로 즉시 회전하는 AnimNotify.
 * Yaw만 회전 (Pitch/Roll 고정) → 보스가 허리를 굽히는 현상 방지.
 */
UCLASS()
class LYRAGAME_API UAN_AutoTargeting : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;
	
	// 회전 허용 최대 각도 (도). 0이면 제한 없음.
	// 예: 90으로 설정 시 이미 등을 돌린 상태에서 반대편 공격은 회전 안 함.
	// 회전 허용 최대 각도 (도). 0이면 제한 없음.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AutoTargeting",
		meta = (ClampMin = "0.0", ClampMax = "360.0"))
	float MaxRotationAngle = 0.f;
};
