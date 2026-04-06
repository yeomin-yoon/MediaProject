// Fill out your copyright notice in the Description page of Project Settings.


#include "Boss/Notify/AN_BossCreateMonster.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Boss/BossCharacterBase.h"


void UAN_BossCreateMonster::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	Super::Notify(MeshComp, Animation);
	ABossCharacterBase* Boss = Cast<ABossCharacterBase>(MeshComp->GetOwner());
	if (!Boss)
	{
		return;
	}
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Boss,FGameplayTag::RequestGameplayTag("Boss.EVENT.CREATE"),FGameplayEventData());
}
