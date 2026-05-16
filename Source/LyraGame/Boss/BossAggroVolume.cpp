#include "Boss/BossAggroVolume.h"

#include "Components/SphereComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameplayStateTreeModule/Public/Components/StateTreeAIComponent.h"
#include "Boss/BossCharacterBase.h"
#include "Boss/BossCharacterBaseAiController.h"

ABossAggroVolume::ABossAggroVolume(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;

	DetectionSphere = ObjectInitializer.CreateDefaultSubobject<USphereComponent>(this, TEXT("DetectionSphere"));
	RootComponent = DetectionSphere;
	DetectionSphere->SetSphereRadius(1500.f);
	DetectionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	DetectionSphere->SetCollisionResponseToAllChannels(ECR_Overlap);
	DetectionSphere->SetGenerateOverlapEvents(true);
	DetectionSphere->SetCanEverAffectNavigation(false); // NavMesh 영향 차단
}

void ABossAggroVolume::BeginPlay()
{
	Super::BeginPlay();

	DetectionSphere->OnComponentBeginOverlap.AddDynamic(this, &ABossAggroVolume::OnDetectionBeginOverlap);
}

void ABossAggroVolume::OnDetectionBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (bAlreadyTriggered) return;

	// 플레이어만 통과
	APawn* OtherPawn = Cast<APawn>(OtherActor);
	if (!OtherPawn || !Cast<APlayerController>(OtherPawn->GetController()))
	{
		return;
	}

	// 월드에 보스 1마리 가정 → 자동 검색
	ABossCharacterBase* Boss = Cast<ABossCharacterBase>(
		UGameplayStatics::GetActorOfClass(GetWorld(), ABossCharacterBase::StaticClass()));
	if (!Boss) return;

	ABossCharacterBaseAiController* BossAI = Cast<ABossCharacterBaseAiController>(Boss->GetController());
	if (!BossAI) return;

	BossAI->RegisterTarget(OtherActor);

	if (UStateTreeAIComponent* STComp = BossAI->GetStateTreeComp())
	{
		STComp->SendStateTreeEvent(
			FStateTreeEvent(FGameplayTag::RequestGameplayTag("Boss.Event.PlayerDetected")));
	}

	UE_LOG(LogTemp, Log, TEXT("[AggroVolume] 플레이어 감지 → 보스 깨움"));

	if (bTriggerOnce)
	{
		bAlreadyTriggered = true;
		DetectionSphere->SetGenerateOverlapEvents(false);
	}
}
