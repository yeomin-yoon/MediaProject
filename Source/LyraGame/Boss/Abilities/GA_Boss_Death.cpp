#include "Boss/Abilities/GA_Boss_Death.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "Boss/BossCharacterBase.h"
#include "GameFramework/CharacterMovementComponent.h"

UGA_Boss_Death::UGA_Boss_Death()
{
	// 부모(ULyraGameplayAbility_Death)에서:
	//  - GameplayEvent.Death 트리거 자동 등록
	//  - bAutoStartDeath=true → ActivateAbility에서 HealthComponent->StartDeath() 자동 호출
	//  - EndAbility에서 FinishDeath() 자동 호출
	//  - 다른 GA CancelAbilities 자동 처리

	// 보스 식별 태그 (필요 시 디버그/검색용)
	AbilityTags.AddTag(FGameplayTag::RequestGameplayTag("Boss.Action.Death"));

	// 활성화 시 보유 태그: 다른 보스 GA들이 ActivationBlockedTags로 잡으면 자동 차단됨
	ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag("Boss.State.Dying"));
}

void UGA_Boss_Death::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	// 부모 ActivateAbility가 StartDeath() 호출 → DeathState 변경 + OnDeathStarted broadcast
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	UE_LOG(LogTemp, Warning, TEXT("[Death] GA_Boss_Death ActivateAbility 진입"));

	CacheHandle = Handle;
	CacheActorInfo = ActorInfo;
	CacheActivationInfo = ActivationInfo;

	ABossCharacterBase* Char = Cast<ABossCharacterBase>(GetAvatarActorFromActorInfo());
	if (!Char)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// AI 정지 (보스 자기 컨트롤러 기준)
	if (AAIController* AICon = Cast<AAIController>(Char->GetController()))
	{
		if (AICon->BrainComponent)
		{
			AICon->BrainComponent->StopLogic(TEXT("Death"));
		}
	}

	// 무브먼트 정지
	Char->GetCharacterMovement()->DisableMovement();

	// 죽음 몽타주 재생
	if (!DeathMontage)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Death] DeathMontage 미지정 → 즉시 종료"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[Death] DeathMontage 재생 시작: %s"), *GetNameSafe(DeathMontage));
	UAbilityTask_PlayMontageAndWait* Task = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, DeathMontage);
	Task->OnCompleted.AddDynamic(this, &ThisClass::OnDeathMontageCompleted);
	Task->OnInterrupted.AddDynamic(this, &ThisClass::OnDeathMontageCancelled);
	Task->OnCancelled.AddDynamic(this, &ThisClass::OnDeathMontageCancelled);
	Task->ReadyForActivation();
}

void UGA_Boss_Death::OnDeathMontageCompleted()
{
	// 부모 EndAbility가 FinishDeath() 호출 → OnDeathFinished broadcast → 캐릭터 정리
	EndAbility(CacheHandle, CacheActorInfo, CacheActivationInfo, true, false);
}

void UGA_Boss_Death::OnDeathMontageCancelled()
{
	EndAbility(CacheHandle, CacheActorInfo, CacheActivationInfo, true, true);
}
