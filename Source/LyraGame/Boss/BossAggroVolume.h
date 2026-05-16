#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BossAggroVolume.generated.h"

class USphereComponent;

UCLASS()
class LYRAGAME_API ABossAggroVolume : public AActor
{
	GENERATED_BODY()

public:
	ABossAggroVolume(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Aggro")
	TObjectPtr<USphereComponent> DetectionSphere;

	UPROPERTY(EditDefaultsOnly, Category = "Aggro")
	bool bTriggerOnce = true;

	UFUNCTION()
	void OnDetectionBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

private:
	bool bAlreadyTriggered = false;
};
