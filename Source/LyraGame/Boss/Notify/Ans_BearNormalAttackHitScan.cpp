#include "Boss/Notify/Ans_BearNormalAttackHitScan.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Kismet/KismetSystemLibrary.h"

void UAns_BearNormalAttackHitScan::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                                               float TotalDuration)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration);
	HitActors.Empty();
}

void UAns_BearNormalAttackHitScan::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float FrameDeltaTime)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime);

	if (MeshComp == nullptr) return;

	AActor* Boss = MeshComp->GetOwner();
	FVector leftHandLocation = MeshComp->GetSocketLocation(FName("hand_l"));
	FVector rightHandLocation = MeshComp->GetSocketLocation(FName("hand_r"));

	TArray<AActor*> IgnoreActors;
	IgnoreActors.Add(Boss);

	TArray<FHitResult> HitResults;
	UKismetSystemLibrary::SphereTraceMulti(Boss, leftHandLocation, leftHandLocation, TraceRadius,
		UEngineTypes::ConvertToTraceType(ECC_Pawn), false, IgnoreActors,
		EDrawDebugTrace::ForOneFrame, HitResults, true);

	TArray<FHitResult> RightHitResults;
	UKismetSystemLibrary::SphereTraceMulti(Boss, rightHandLocation, rightHandLocation, TraceRadius,
		UEngineTypes::ConvertToTraceType(ECC_Pawn), false, IgnoreActors,
		EDrawDebugTrace::ForOneFrame, RightHitResults, true);

	HitResults.Append(RightHitResults);

	for (const FHitResult& Hit : HitResults)
	{
		AActor* HitActor = Hit.GetActor();
		if (!HitActor || HitActors.Contains(HitActor)) continue;

		HitActors.Add(HitActor);

		if (!DamageEffectClass) continue;

		UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Boss);
		UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitActor);

		if (!SourceASC || !TargetASC) continue;

		FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, SourceASC->MakeEffectContext());
		SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data, TargetASC);
	}
}

void UAns_BearNormalAttackHitScan::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	Super::NotifyEnd(MeshComp, Animation);
	HitActors.Empty();
}
