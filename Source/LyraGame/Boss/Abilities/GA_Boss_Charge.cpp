#include "Boss/Abilities/GA_Boss_Charge.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Boss/BossCharacterBaseAiController.h"
#include "Boss/Bear/BearBossBase.h"
#include "GameFramework/CharacterMovementComponent.h"

UGA_Boss_Charge::UGA_Boss_Charge()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	AbilityTags.AddTag(FGameplayTag::RequestGameplayTag("Boss.Attack.Charge")); 
	ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag("Boss.Attack.Charge")); // 활성화시 위 태그를 가짐
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag("Boss.Attack.Charge"));// 이 태그가 활성화 되잇는중에는 실행이 안되게
	
}

void UGA_Boss_Charge::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	UE_LOG(LogTemp, Warning, TEXT("[GA_Charge] ActivateAbility 진입"));
	CacheHandle = Handle;
	CacheActorInfo = ActorInfo;
	CacheActivationInfo = ActivationInfo;
	if (!CanActivateAbility(Handle, ActorInfo, nullptr,nullptr,nullptr))
	{
		UE_LOG(LogTemp, Warning, TEXT("[GA_Charge] CanActivateAbility 실패"));
		EndAbility(Handle,ActorInfo,ActivationInfo,true,false);
		return;
	}
	AActor* RawActor = ActorInfo->AvatarActor.Get();
	ABearBossBase* BossCharacter = Cast<ABearBossBase>(RawActor);
	if (!BossCharacter)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GA_Charge] BossCharacter 캐스팅 실패"));
		EndAbility(Handle,ActorInfo,ActivationInfo,true,false);
		return;
	}
	ABossCharacterBaseAiController* BossCharacterBaseAiController = Cast<ABossCharacterBaseAiController>(BossCharacter->Controller);
	if (!BossCharacterBaseAiController)
	{
		EndAbility(Handle,ActorInfo,ActivationInfo,true,false);
		return;
	}
	AActor* TargetActor = BossCharacterBaseAiController->GetNearestTarget();
	if (!TargetActor)
	{
		EndAbility(Handle,ActorInfo,ActivationInfo,true,false);
		return;
	}
	FVector Dir = (TargetActor->GetActorLocation() - BossCharacter->GetActorLocation()).GetSafeNormal();
	BossCharacter->bIsCharge = true;
	BossCharacter->GetCharacterMovement()->GroundFriction = 0.f;
	BossCharacter->GetCharacterMovement()->BrakingDecelerationWalking = 0.f;
	BossCharacter->LaunchCharacter(Dir * ChargeSpd, true, true);
	//지정된 시간동안만 유지하기 OR 부딫히면 종료
	
	GetWorld()->GetTimerManager().SetTimer(TimerHandle,[this,Handle,ActorInfo,ActivationInfo,BossCharacter]()
	{
		BossCharacter->ChargeEnd();
		EndAbility(Handle,ActorInfo,ActivationInfo,true,false);
	},6.0f,false);
	BossCharacter->OnActorHit.AddDynamic(this,&UGA_Boss_Charge::OnChargeHit);
	
	
}

void UGA_Boss_Charge::OnChargeHit(AActor* SelfActor, AActor* OtherActor, FVector NormalImpulse, const FHitResult& Hit)
{
	UE_LOG(LogTemp, Warning, TEXT("[GA_Charge] OnChargeHit: %s"), *GetNameSafe(OtherActor));
	if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OtherActor))
	{
		if (ASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag("Character.Player")))
		{
			UE_LOG(LogTemp, Warning, TEXT("[GA_Charge] 플레이어 히트 → EndAbility"));
			EndAbility(CacheHandle,CacheActorInfo,CacheActivationInfo,true,false);
		}
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
	GetWorld()->GetTimerManager().ClearTimer(TimerHandle);
	BossBase->ChargeEnd();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

bool UGA_Boss_Charge::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	AActor* Avartar = ActorInfo->AvatarActor.Get();
	ABossCharacterBase* BossCharacter = Cast<ABossCharacterBase>(Avartar);
	if (!BossCharacter) return false;
	return BossCharacter->GetCharacterMovement()->IsMovingOnGround();
}


