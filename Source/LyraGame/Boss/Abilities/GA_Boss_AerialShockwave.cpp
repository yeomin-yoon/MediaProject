#include "Boss/Abilities/GA_Boss_AerialShockwave.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Boss/BossCharacterBase.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetSystemLibrary.h"

UGA_Boss_AerialShockwave::UGA_Boss_AerialShockwave()
{
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

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ABossCharacterBase* Boss = Cast<ABossCharacterBase>(GetAvatarActorFromActorInfo());
	if (!Boss)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	Boss->SetIsJumping(true);
	Boss->SetIsGrounded(false);

	Boss->LaunchCharacter(FVector::UpVector * LaunchPower, false, false);

	FVector SpawnLocation;
	if (Boss->GetCharacterMovement()->CurrentFloor.IsWalkableFloor())
	{
		FVector FootLocation = Boss->GetActorLocation();
		FootLocation.Z -= 2 * Boss->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		SpawnLocation = FootLocation;
	}
	UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), JumpStartFX, SpawnLocation, FRotator::ZeroRotator, FVector(5.0f));

	Boss->OnLandedDelegate.AddUObject(this, &UGA_Boss_AerialShockwave::OnBossLanded);
}

void UGA_Boss_AerialShockwave::OnBossLanded()
{
	ABossCharacterBase* Boss = Cast<ABossCharacterBase>(GetAvatarActorFromActorInfo());
	if (!Boss)
	{
		EndAbility(CacheHandle, CacheActorInfo, CacheActivationInfo, true, true);
		return;
	}

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
	ABossCharacterBase* Boss = Cast<ABossCharacterBase>(GetAvatarActorFromActorInfo());
	if (Boss)
	{
		Boss->OnLandedDelegate.RemoveAll(this);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
