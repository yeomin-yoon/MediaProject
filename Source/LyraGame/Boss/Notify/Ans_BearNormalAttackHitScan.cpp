#include "Boss/Notify/Ans_BearNormalAttackHitScan.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Kismet/KismetSystemLibrary.h"

void UAns_BearNormalAttackHitScan::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                                               float TotalDuration)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration);

	// TODO: HitActors Set 초기화
	// 힌트: HitActors.Empty()
	HitActors.Empty();
}

void UAns_BearNormalAttackHitScan::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float FrameDeltaTime)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime);

	// TODO: MeshComp가 유효한지 체크, 없으면 return
	if (MeshComp==nullptr) return;
	// TODO: MeshComp->GetOwner()로 보스 액터 가져오기
	AActor* Boss = MeshComp->GetOwner();

	// TODO: 왼손 소켓 위치 가져오기
	// 힌트: MeshComp->GetSocketLocation(FName("hand_l"))
	FVector leftHandLocation = MeshComp->GetSocketLocation(FName("hand_l"));

	// TODO: 오른손 소켓 위치 가져오기
	// 힌트: MeshComp->GetSocketLocation(FName("hand_r"))
	FVector rightHandLocation = MeshComp->GetSocketLocation(FName("hand_r"));

	TArray<AActor*> IgnoreActors;
	IgnoreActors.Add(Boss);

	// 왼손, 오른손 각각 트레이스
	TArray<FHitResult> HitResults;
	UKismetSystemLibrary::SphereTraceMulti(Boss, leftHandLocation, leftHandLocation, TraceRadius,
		UEngineTypes::ConvertToTraceType(ECC_Pawn), false, IgnoreActors,
		EDrawDebugTrace::ForDuration, HitResults, true);

	TArray<FHitResult> RightHitResults;
	UKismetSystemLibrary::SphereTraceMulti(Boss, rightHandLocation, rightHandLocation, TraceRadius,
		UEngineTypes::ConvertToTraceType(ECC_Pawn), false, IgnoreActors,
		EDrawDebugTrace::ForDuration, RightHitResults, true);

	HitResults.Append(RightHitResults);

	// 히트된 액터 순회
	for (const FHitResult& Hit : HitResults)
	{
		AActor* HitActor = Hit.GetActor();

		// 유효하지 않거나 이미 히트한 액터면 스킵
		if (!HitActor || HitActors.Contains(HitActor)) continue;

		HitActors.Add(HitActor);

		// TODO: DamageEffectClass가 유효한지 체크, 없으면 continue
		if (!DamageEffectClass)
		{
			continue;
		}
		
		// TODO: 보스(Boss)의 ASC 가져오기
		//       힌트: UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Boss)
		//       타입: UAbilitySystemComponent*
		UAbilitySystemComponent* SourceAbilitySystemComponent = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Boss);
		
		// TODO: 타겟(HitActor)의 ASC 가져오기
		//       힌트: UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitActor)
		//       타입: UAbilitySystemComponent*
		UAbilitySystemComponent* TargetAbilitySystemComponent = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitActor);
		

		// TODO: 둘 다 유효한지 체크, 아니면 continue
		if (!TargetAbilitySystemComponent || !SourceAbilitySystemComponent)
		{
			continue;
		}
		// TODO: GE Spec 생성 후 타겟에 적용
		//       1단계: FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, SourceASC->MakeEffectContext())
		//       2단계: SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data, TargetASC)
		FGameplayEffectSpecHandle Spec=  SourceAbilitySystemComponent->MakeOutgoingSpec(DamageEffectClass, 1.0f,SourceAbilitySystemComponent->MakeEffectContext());
		SourceAbilitySystemComponent->ApplyGameplayEffectSpecToTarget(*Spec.Data, TargetAbilitySystemComponent);
	}
}

void UAns_BearNormalAttackHitScan::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	Super::NotifyEnd(MeshComp, Animation);

	// TODO: HitActors Set 정리
	HitActors.Empty();
}
