#include "AN_AutoTargeting.h"

#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"

void UAN_AutoTargeting::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp) return;

	AActor* Owner = MeshComp->GetOwner();
	if (!Owner) return;

	UWorld* World = Owner->GetWorld();
	if (!World) return;

	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC) return;

	APawn* PlayerPawn = PC->GetPawn();
	if (!PlayerPawn) return;

	FVector ToTarget = PlayerPawn->GetActorLocation() - Owner->GetActorLocation();
	ToTarget.Z = 0.f;

	if (ToTarget.IsNearlyZero()) return;

	if (MaxRotationAngle > 0.f)
	{
		const FVector Forward = Owner->GetActorForwardVector();
		const float DotProduct = FVector::DotProduct(Forward, ToTarget.GetSafeNormal());
		const float AngleDeg = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(DotProduct, -1.f, 1.f)));

		if (AngleDeg > MaxRotationAngle) return;
	}

	const FRotator NewRotation(0.f, ToTarget.Rotation().Yaw, 0.f);
	Owner->SetActorRotation(NewRotation);
}

FString UAN_AutoTargeting::GetNotifyName_Implementation() const
{
	return TEXT("AutoTargeting");
}
