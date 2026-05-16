#include "Boss/Abilities/GA_Boss_Charge.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "ActionCombatReactionComponent.h"
#include "GA_BossStuan.h"
#include "Boss/BossCharacterBaseAiController.h"
#include "Boss/Bear/BearBossBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"

UGA_Boss_Charge::UGA_Boss_Charge()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	AbilityTags.AddTag(FGameplayTag::RequestGameplayTag("Boss.Attack.Charge"));
	ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag("Boss.Attack.Charge")); // 활성화시 위 태그를 가짐
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag("Boss.Attack.Charge"));// 이 태그가 활성화 되잇는중에는 실행이 안되게
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag("Boss.State.Stunned"));// 스턴 중 발동 차단
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag("Boss.State.Dying"));// 죽음 중 발동 차단
	
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
	FVector Dir = TargetActor->GetActorLocation() - BossCharacter->GetActorLocation();
	Dir.Z = 0.f;
	Dir = Dir.GetSafeNormal();
	if (Dir.IsNearlyZero())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	FRotator LookRot = Dir.Rotation();
	LookRot.Pitch = 0.f;
	LookRot.Roll = 0.f;
	BossCharacter->SetActorRotation(LookRot);

	BossCharacter->bIsCharge = true;
	BossCharacter->GetCharacterMovement()->GroundFriction = 0.f;
	BossCharacter->GetCharacterMovement()->BrakingDecelerationWalking = 0.f;
	BossCharacter->LaunchCharacter(Dir * ChargeSpd, true, false);
	
	
	
	GetWorld()->GetTimerManager().SetTimer(TimerHandle,[this,Handle,ActorInfo,ActivationInfo,BossCharacter]()
	{
		BossCharacter->ChargeEnd();
		EndAbility(Handle,ActorInfo,ActivationInfo,true,false);
	},ChargeSecond,false);
	BossCharacter->OnActorHit.AddDynamic(this,&UGA_Boss_Charge::OnChargeHit);
	BossCharacter->OnActorBeginOverlap.AddDynamic(this,&UGA_Boss_Charge::OnChargeOverlap);
	
	
}

void UGA_Boss_Charge::OnChargeHit(AActor* SelfActor, AActor* OtherActor, FVector NormalImpulse, const FHitResult& Hit)
{
	UE_LOG(LogTemp, Warning, TEXT("[Stun] OnChargeHit 진입 - OtherActor: %s"), *GetNameSafe(OtherActor));

	APawn* OtherPawn = Cast<APawn>(OtherActor);
	if (OtherPawn && Cast<APlayerController>(OtherPawn->GetController()))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Stun] 플레이어 히트"));
		UActionCombatReactionComponent::ForceKnockdownActor(OtherActor, SelfActor);
		EndAbility(CacheHandle, CacheActorInfo, CacheActivationInfo, true, false);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[Stun] 플레이어 아님 → 무시 (Actor: %s)"), *GetNameSafe(OtherActor));
	}
}

void UGA_Boss_Charge::OnChargeOverlap(AActor* SelfActor, AActor* OtherActor)
{
	UAbilitySystemComponent* OtherASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OtherActor);
	if (!OtherASC) return;

	if (OtherASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag("Actor.Wall")))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Charge] 벽 히트 → Charge 종료 + Stun 이벤트 발송"));
		EndAbility(CacheHandle, CacheActorInfo, CacheActivationInfo, true, false);

		
		if (ABearBossBase* BossPawn = Cast<ABearBossBase>(SelfActor))
		{
			if (ABossCharacterBaseAiController* AIC = Cast<ABossCharacterBaseAiController>(BossPawn->GetController()))
			{
				if (UStateTreeAIComponent* STComp = AIC->GetStateTreeComp())
				{
					STComp->SendStateTreeEvent(
						FStateTreeEvent(FGameplayTag::RequestGameplayTag("Boss.Event.Stun")));
					UE_LOG(LogTemp, Warning, TEXT("[Charge] Boss.Event.Stun 이벤트 발송"));
				}
			}
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


