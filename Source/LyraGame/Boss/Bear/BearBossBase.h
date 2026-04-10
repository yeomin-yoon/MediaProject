#pragma once

#include "CoreMinimal.h"
#include "Boss/BossCharacterBase.h"
#include "BearBossBase.generated.h"

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

	FOnBossLanded OnLandedDelegate;

protected:
	virtual void Landed(const FHitResult& Hit) override;
};
