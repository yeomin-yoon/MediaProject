#include "Boss/Abilities/GA_Boss_Charge.h"

#include "AbilitySystemComponent.h"
#include "Boss/BossCharacterBaseAiController.h"
#include "Boss/Bear/BearBossBase.h"
#include "GameFramework/CharacterMovementComponent.h"

UGA_Boss_Charge::UGA_Boss_Charge()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UGA_Boss_Charge::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
		
	CacheHandle = Handle;
	CacheActorInfo = ActorInfo;
	CacheActivationInfo= ActivationInfo;
	
	AActor* RawActor = GetAvatarActorFromActorInfo();
	ABearBossBase* BearBossBase = Cast<ABearBossBase>(RawActor);
	if (!BearBossBase)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo,true,false);
		return;
	}
	
	
	ABossCharacterBaseAiController* BearBossController = Cast<ABossCharacterBaseAiController>(BearBossBase->Controller);
	if (!BearBossController)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo,true,false);
		return;
	}
	FVector TargetLocation = BearBossController->GetNearestTarget()->GetActorLocation();
	FVector Dir = (TargetLocation-BearBossBase->GetActorLocation()).GetSafeNormal();
	
	BearBossBase->ChargeStart();
	BearBossBase->LaunchCharacter(Dir*BearBossBase->ChargeSpd,true,true);

	
	BearBossBase->OnActorHit.AddDynamic(this,&UGA_Boss_Charge::OnChargeHit);
}

void UGA_Boss_Charge::OnChargeHit(AActor* SelfActor, AActor* OtherActor, FVector NormalImpulse, const FHitResult& Hit)
{
	ILyraTeamAgentInterface* TeamAgent = Cast<ILyraTeamAgentInterface>(OtherActor); // 팀id 비교 
	
	if (TeamAgent != nullptr && TeamAgent->GetGenericTeamId() == 1)// 플레이어
	{
		EndAbility(CacheHandle,CacheActorInfo,CacheActivationInfo,true,false);
		return;
	}
	else if ( TeamAgent != nullptr && TeamAgent->GetGenericTeamId() == 3)//벽이나 지형지물
	{
		EndAbility(CacheHandle,CacheActorInfo,CacheActivationInfo,true,false);
		return;
	}
	
}

void UGA_Boss_Charge::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	
	ABearBossBase* BossBase  = Cast<ABearBossBase>(GetAvatarActorFromActorInfo());
	UAbilitySystemComponent* SourceAsc = BossBase->GetAbilitySystemComponent();
	BossBase->OnActorHit.RemoveDynamic(this,&UGA_Boss_Charge::OnChargeHit);
	SourceAsc->RemoveLooseGameplayTag((FGameplayTag::RequestGameplayTag("Boss.State.Charging")));
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}


