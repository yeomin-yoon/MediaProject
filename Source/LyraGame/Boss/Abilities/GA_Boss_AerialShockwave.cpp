#include "Boss/Abilities/GA_Boss_AerialShockwave.h"
#include "Boss/BossCharacterBase.h"
#include "Kismet/KismetSystemLibrary.h"

// TODO: 범위 데미지 적용에 필요한 헤더 추가
// 이유: SphereOverlapActors, ApplyGameplayEffectToTarget 등을 쓰려면
//       각각의 헤더가 필요함. 실제 구현 시점에 추가할 것.
// #include "Kismet/GameplayStatics.h"
// #include "AbilitySystemComponent.h"

UGA_Boss_AerialShockwave::UGA_Boss_AerialShockwave()
{
	// TODO: InstancingPolicy 설정
	
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UGA_Boss_AerialShockwave::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	CacheHandle = Handle;
	CacheActivationInfo = ActivationInfo;
	CacheActorInfo = ActorInfo;
	// TODO: CommitAbility 호출
	
	if (!CommitAbility(Handle,ActorInfo,ActivationInfo))
	{
		EndAbility(Handle, ActorInfo,ActivationInfo,true,true);
		return;
	}

	ABossCharacterBase* Boss = Cast<ABossCharacterBase>(GetAvatarActorFromActorInfo());
	if (!Boss)
	{
		// TODO: EndAbility 호출 (실패 처리)
		
		EndAbility(Handle, ActorInfo,ActivationInfo,true,true);
		return;
	}

	// TODO: 점프 변수 세팅 (bIsJumping = true, bIsGrounded = false)
	
	Boss->SetIsJumping(true);
	Boss->SetIsGrounded(false);

	// TODO: LaunchCharacter 호출

	FVector dir = FVector::UpVector;
	Boss->LaunchCharacter(dir*LaunchPower,false,false);

	// TODO: 델리게이트 바인딩
	
	Boss->OnLandedDelegate.AddUObject(this, &UGA_Boss_AerialShockwave::OnBossLanded);
}

void UGA_Boss_AerialShockwave::OnBossLanded()
{
	// TODO: 범위 내 액터 탐지 (SphereOverlapActors)

	ABossCharacterBase* Boss = Cast<ABossCharacterBase>(GetAvatarActorFromActorInfo());
	if (!Boss) { return; }  

	
	TArray<AActor*> OverlapResult;
	UKismetSystemLibrary::SphereOverlapActors(
		GetWorld(),
		Boss->GetActorLocation(),
		HitRadius,
		TArray<TEnumAsByte<EObjectTypeQuery>>(),
		nullptr,
		TArray<AActor*>{Boss},  
		OverlapResult);

	TSet<AActor*> HitActors(OverlapResult);
	// TODO: 탐지된 액터에 GE 적용

	// TODO: EndAbility 호출
	EndAbility(CacheHandle, CacheActorInfo, CacheActivationInfo, true, false);
}

void UGA_Boss_AerialShockwave::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{

	ABossCharacterBase* Boss = Cast<ABossCharacterBase>(GetAvatarActorFromActorInfo());
	if (Boss)
	{
		Boss->OnLandedDelegate.RemoveAll(this);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
