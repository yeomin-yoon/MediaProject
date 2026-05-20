// Copyright Epic Games, Inc. All Rights Reserved.

#include "LyraWorldCollectable.h"

#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Components/BoxComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "UObject/ConstructorHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraWorldCollectable)

struct FInteractionQuery;

ALyraWorldCollectable::ALyraWorldCollectable()
{
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem>
		RedNSObj(
			TEXT("/Script/Niagara.NiagaraSystem'/Game/Loot_Drop_VFX/Niagara/NS_Imortal_Loot_Drop_Red.NS_Imortal_Loot_Drop_Red'")
		);

	if (RedNSObj.Succeeded())
	{
		RedNS = RedNSObj.Object;
	}

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem>
		YellowNSObj(
			TEXT("/Script/Niagara.NiagaraSystem'/Game/Loot_Drop_VFX/Niagara/NS_Imortal_Loot_Drop_Yellow.NS_Imortal_Loot_Drop_Yellow'")
		);

	if (YellowNSObj.Succeeded())
	{
		YellowNS = YellowNSObj.Object;
	}

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem>
		BlueNSObj(
			TEXT("/Script/Niagara.NiagaraSystem'/Game/Loot_Drop_VFX/Niagara/NS_Imortal_Loot_Drop_Blue.NS_Imortal_Loot_Drop_Blue'")
		);

	if (BlueNSObj.Succeeded())
	{
		BlueNS = BlueNSObj.Object;
	}
	
	BoxComp = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxComponent"));
	SetRootComponent(BoxComp);
	BoxComp->SetBoxExtent(FVector(35.f, 35.f, 51.f));
	
	NiagaraComp = CreateDefaultSubobject<UNiagaraComponent>(TEXT("NiagaraComponent"));
	NiagaraComp->SetupAttachment(BoxComp);
	NiagaraComp->SetRelativeLocation(FVector(0.f, 0.f, -32.8f));
	
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	
	ProjectileMovement->InitialSpeed = 0.f;
	ProjectileMovement->MaxSpeed = 2000.f;

	ProjectileMovement->ProjectileGravityScale = 2.0f;

	ProjectileMovement->bRotationFollowsVelocity = false;
	ProjectileMovement->bShouldBounce = true;

	ProjectileMovement->Bounciness = 0.35f;
	ProjectileMovement->Friction = 0.6f;
	ProjectileMovement->BounceVelocityStopSimulatingThreshold = 60.f;
}

void ALyraWorldCollectable::GatherInteractionOptions(const FInteractionQuery& InteractQuery, FInteractionOptionBuilder& InteractionBuilder)
{
	InteractionBuilder.AddInteractionOption(Option);
}

FInventoryPickup ALyraWorldCollectable::GetPickupInventory() const
{
	FInventoryPickup Result = StaticInventory;

	for (FPickupTemplate& Template : Result.Templates)
	{
		Template.RandomSeed = RandomSeed;
		Template.OptionType = OptionType;
		Template.Rarity = Rarity;
	}

	return Result;
}

void ALyraWorldCollectable::LaunchItem(FVector Velocity)
{
	if (ProjectileMovement)
	{
		ProjectileMovement->StopMovementImmediately();
		ProjectileMovement->Velocity = Velocity;
		ProjectileMovement->UpdateComponentVelocity();
		ProjectileMovement->Activate(true);
	}
}

void ALyraWorldCollectable::ApplyNiagaraByOption()
{
	if (!NiagaraComp)
		return;

	UNiagaraSystem* SelectedSystem = nullptr;

	switch (OptionType)
	{
	case EItemOptionType::Attack:
		SelectedSystem = RedNS;
		break;

	case EItemOptionType::Health:
		SelectedSystem = YellowNS;
		break;

	case EItemOptionType::Stamina:
		SelectedSystem = BlueNS;
		break;
	}

	if (SelectedSystem)
	{
		NiagaraComp->SetAsset(SelectedSystem);
	}
}