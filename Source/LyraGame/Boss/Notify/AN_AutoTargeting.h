#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AN_AutoTargeting.generated.h"

UCLASS()
class LYRAGAME_API UAN_AutoTargeting : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AutoTargeting",
		meta = (ClampMin = "0.0", ClampMax = "360.0"))
	float MaxRotationAngle = 0.f;
};
