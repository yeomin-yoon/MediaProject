#include "Boss/Abilities/GA_Boss_AerialShockwave.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Boss/Bear/BearBossBase.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "TimerManager.h"
#include "Engine/World.h"

UGA_Boss_AerialShockwave::UGA_Boss_AerialShockwave()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag("Boss.State.Stunned"));
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag("Boss.State.Dying"));
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

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ABearBossBase* Boss = Cast<ABearBossBase>(GetAvatarActorFromActorInfo());
	if (!Boss)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	{
		UCharacterMovementComponent* MoveComp = Boss->GetCharacterMovement();
		UE_LOG(LogTemp, Warning,
			TEXT("[Aerial] ActivateAbility 진입 - MovementMode=%d (0=None,1=Walk,3=Fall) IsFalling=%d IsMovingOnGround=%d Velocity=%s"),
			(int32)MoveComp->MovementMode.GetValue(),
			MoveComp->IsFalling(),
			MoveComp->IsMovingOnGround(),
			*MoveComp->Velocity.ToString());
	}

	Boss->SetIsJumping(true);
	Boss->SetIsGrounded(false);

	Boss->LaunchCharacter(FVector::UpVector * LaunchPower, false, false);

	{
		UCharacterMovementComponent* MoveComp = Boss->GetCharacterMovement();
		UE_LOG(LogTemp, Warning,
			TEXT("[Aerial] LaunchCharacter 직후 - MovementMode=%d IsFalling=%d IsMovingOnGround=%d Velocity=%s"),
			(int32)MoveComp->MovementMode.GetValue(),
			MoveComp->IsFalling(),
			MoveComp->IsMovingOnGround(),
			*MoveComp->Velocity.ToString());
	}

	FVector SpawnLocation;
	if (Boss->GetCharacterMovement()->CurrentFloor.IsWalkableFloor())
	{
		FVector FootLocation = Boss->GetActorLocation();
		FootLocation.Z -= 2 * Boss->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		SpawnLocation = FootLocation;
	}
	UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), JumpStartFX, SpawnLocation, FRotator::ZeroRotator, FVector(5.0f));

	Boss->OnLandedDelegate.AddUObject(this, &UGA_Boss_AerialShockwave::OnBossLanded);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			SafetyTimerHandle, this, &UGA_Boss_AerialShockwave::OnSafetyTimeout, SafetyTimeout, false);
	}
}

void UGA_Boss_AerialShockwave::OnSafetyTimeout()
{
	UE_LOG(LogTemp, Warning, TEXT("[Aerial] SafetyTimeout 만료 → Landed 미수신, 강제 착지 처리"));
	OnBossLanded();
}

void UGA_Boss_AerialShockwave::OnBossLanded()
{
	UE_LOG(LogTemp, Warning, TEXT("[Aerial] OnBossLanded 진입 (Landed 델리게이트 정상 수신)"));
	ABearBossBase* Boss = Cast<ABearBossBase>(GetAvatarActorFromActorInfo());
	if (!Boss)
	{
		EndAbility(CacheHandle, CacheActorInfo, CacheActivationInfo, true, true);
		return;
	}

	// 안전망 경로(엔진 Landed 미호출)에서 ABP 점프 플래그가 그대로 남는 문제 방지
	Boss->SetIsJumping(false);
	Boss->SetIsGrounded(true);

	TArray<AActor*> OverlapResult;
	UKismetSystemLibrary::SphereOverlapActors(
		GetWorld(),
		Boss->GetActorLocation(),
		HitRadius,
		TArray<TEnumAsByte<EObjectTypeQuery>>(),
		nullptr,
		TArray<AActor*>{Boss},
		OverlapResult);

	FVector SpawnLocation;
	if (Boss->GetCharacterMovement()->CurrentFloor.IsWalkableFloor())
	{
		FVector FootLocation = Boss->GetActorLocation();
		FootLocation.Z -= 2 * Boss->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		SpawnLocation = FootLocation;
	}
	UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), JumpEndFX, SpawnLocation, FRotator::ZeroRotator, FVector(5.0f));

	UAbilitySystemComponent* BossASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Boss);
	if (!BossASC || !DamageEffectClass)
	{
		EndAbility(CacheHandle, CacheActorInfo, CacheActivationInfo, true, true);
		return;
	}

	FGameplayEffectSpecHandle Spec = BossASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, BossASC->MakeEffectContext());
	for (AActor* Actor : OverlapResult)
	{
		UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Actor);
		if (!TargetASC) continue;
		BossASC->ApplyGameplayEffectSpecToTarget(*Spec.Data, TargetASC);
	}

	EndAbility(CacheHandle, CacheActorInfo, CacheActivationInfo, true, false);
}

void UGA_Boss_AerialShockwave::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	UE_LOG(LogTemp, Warning, TEXT("[Aerial] EndAbility 호출 - bWasCancelled=%d"), bWasCancelled);
	ABearBossBase* Boss = Cast<ABearBossBase>(GetAvatarActorFromActorInfo());
	if (Boss)
	{
		Boss->OnLandedDelegate.RemoveAll(this);
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SafetyTimerHandle);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
