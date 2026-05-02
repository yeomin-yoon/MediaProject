#include "Boss/Abilities/GA_Boss_Charge.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GA_BossStuan.h"
#include "Boss/BossCharacterBaseAiController.h"
#include "Boss/Bear/BearBossBase.h"
#include "GameFramework/CharacterMovementComponent.h"

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
	FVector Dir = (TargetActor->GetActorLocation() - BossCharacter->GetActorLocation()).GetSafeNormal();
	FRotator LookRot = Dir.Rotation();
	BossCharacter->SetActorRotation(LookRot);
	BossCharacter->bIsCharge = true;
	BossCharacter->GetCharacterMovement()->GroundFriction = 0.f;
	BossCharacter->GetCharacterMovement()->BrakingDecelerationWalking = 0.f;
	BossCharacter->LaunchCharacter(Dir * ChargeSpd, true, true);
	
	
	
	GetWorld()->GetTimerManager().SetTimer(TimerHandle,[this,Handle,ActorInfo,ActivationInfo,BossCharacter]()
	{
		BossCharacter->ChargeEnd();
		EndAbility(Handle,ActorInfo,ActivationInfo,true,false);
	},ChargeSecond,false);
	BossCharacter->OnActorHit.AddDynamic(this,&UGA_Boss_Charge::OnChargeHit);
	
	
}

void UGA_Boss_Charge::OnChargeHit(AActor* SelfActor, AActor* OtherActor, FVector NormalImpulse, const FHitResult& Hit)
{
	UE_LOG(LogTemp, Warning, TEXT("[Stun] OnChargeHit 진입 - OtherActor: %s"), *GetNameSafe(OtherActor));

	UAbilitySystemComponent* OtherASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OtherActor);
	if (OtherASC)
	{
		FGameplayTagContainer OwnedTags;
		OtherASC->GetOwnedGameplayTags(OwnedTags);
		UE_LOG(LogTemp, Warning, TEXT("[Stun] OtherActor ASC 발견 - 보유 태그: %s"), *OwnedTags.ToString());

		if (OtherASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag("Character.Player")))
		{
			UE_LOG(LogTemp, Warning, TEXT("[Stun] 플레이어 히트"));
			EndAbility(CacheHandle,CacheActorInfo,CacheActivationInfo,true,false);
		}
		else if(OtherASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag("Actor.Wall")))
		{
			UE_LOG(LogTemp, Warning, TEXT("[Stun] 벽 히트 → 스턴 발동 시도"));
			UAbilitySystemComponent* BossASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(SelfActor);
			EndAbility(CacheHandle,CacheActorInfo,CacheActivationInfo,true,false);

			// 디버그: EndAbility 후 보스 ASC 상태 확인
			FGameplayTagContainer BossTags;
			BossASC->GetOwnedGameplayTags(BossTags);
			UE_LOG(LogTemp, Warning, TEXT("[Stun] EndAbility 후 보스 보유 태그: %s"), *BossTags.ToString());

			// 디버그: Grant된 GA 목록 확인
			for (const FGameplayAbilitySpec& Spec : BossASC->GetActivatableAbilities())
			{
				UE_LOG(LogTemp, Warning, TEXT("[Stun] Grant된 GA: %s | AbilityTags: %s"),
					*GetNameSafe(Spec.Ability), *Spec.Ability->AbilityTags.ToString());
			}

			FGameplayTagContainer StunTag;
			StunTag.AddTag(FGameplayTag::RequestGameplayTag("Boss.Action.Stun"));
			bool bSuccess = BossASC->TryActivateAbilitiesByTag(StunTag);
			UE_LOG(LogTemp, Warning, TEXT("[Stun] TryActivateAbilitiesByTag 결과: %s"), bSuccess ? TEXT("성공") : TEXT("실패"));
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[Stun] ASC 있지만 Player/Wall 태그 없음 → 무시"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[Stun] OtherActor에 ASC 없음 → 무시 (Actor: %s)"), *GetNameSafe(OtherActor));
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


