#pragma once

#include "CoreMinimal.h"
#include "Boss/BossCharacterBase.h"
#include "BearBossBase.generated.h"

// 착지 이벤트 델리게이트 — GA_Boss_AerialShockwave가 바인딩해 착지 타이밍을 감지
DECLARE_MULTICAST_DELEGATE(FOnBossLanded);

UCLASS()
class LYRAGAME_API ABearBossBase : public ABossCharacterBase
{
	GENERATED_BODY()

public:
	ABearBossBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Boss|Abilities")
	bool bIsGrounded = true;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Boss|Abilities")
	bool bIsJumping = false;

	bool GetIsGrounded();
	bool GetIsJumping();
	void SetIsGrounded(bool IsGrounded);
	void SetIsJumping(bool IsJumping);

	// GA가 여기에 바인딩하고, Landed()에서 Broadcast — UPROPERTY 불필요(UObject 아님)
	FOnBossLanded OnLandedDelegate;

protected:
	// 물리 기반 착지 감지 → OnLandedDelegate Broadcast
	virtual void Landed(const FHitResult& Hit) override;
};
