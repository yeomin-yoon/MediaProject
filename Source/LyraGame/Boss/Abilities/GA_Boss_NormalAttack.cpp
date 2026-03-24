#include "Boss/Abilities/GA_Boss_NormalAttack.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"

UGA_Boss_NormalAttack::UGA_Boss_NormalAttack()
{
	// TODO: InstancingPolicy 설정
	// 힌트: GA는 인스턴스가 필요함. EGameplayAbilityInstancingPolicy 확인
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor; 
}

void UGA_Boss_NormalAttack::InitializeForCahe(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	CacheHandle = Handle;
	CacheActorInfo = ActorInfo;
	CacheActivationInfo = ActivationInfo;
}

void UGA_Boss_NormalAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                            const FGameplayAbilityActorInfo* ActorInfo,
                                            const FGameplayAbilityActivationInfo ActivationInfo,
                                            const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	InitializeForCahe(Handle, ActorInfo, ActivationInfo);
	
	// TODO: AttackMontage가 유효한지 체크
	// 유효하지 않으면 EndAbility 호출하고 return
	if (AttackMontage == nullptr)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// TODO: UAbilityTask_PlayMontageAndWait* Task 변수 선언 후
	//       CreatePlayMontageAndWaitProxy(this, NAME_None, AttackMontage, 1.0f) 로 생성
	UAbilityTask_PlayMontageAndWait* Task=  UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this,NAME_None,AttackMontage,1.0f);
	// TODO: Task->OnCompleted 에 OnMontageCompleted 바인딩
	//       Task->OnCancelled 에 OnMontageCancelled 바인딩
	//       Task->OnInterrupted 에 OnMontageCancelled 바인딩
	//       힌트: AddDynamic() 사용
	Task->OnCompleted.AddDynamic(this,&UGA_Boss_NormalAttack::OnMontageCompleted);
	Task->OnCancelled.AddDynamic(this,&UGA_Boss_NormalAttack::OnMontageCancelled);
	Task->OnInterrupted.AddDynamic(this,&UGA_Boss_NormalAttack::OnMontageCancelled);
	

	// TODO: Task->ReadyForActivation() 호출
	Task->ReadyForActivation();
}

void UGA_Boss_NormalAttack::OnMontageCompleted()
{
	// TODO: 정상적으로 끝났을 때 EndAbility 호출
	// bWasCancelled = false
	EndAbility(CacheHandle,CacheActorInfo,CacheActivationInfo,true,false);
	
}

void UGA_Boss_NormalAttack::OnMontageCancelled()
{
	// TODO: 취소됐을 때 EndAbility 호출
	// bWasCancelled = true
	EndAbility(CacheHandle,CacheActorInfo,CacheActivationInfo,true,true);
}

void UGA_Boss_NormalAttack::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
