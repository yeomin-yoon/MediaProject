#include "Boss/Bear/BearBossBase.h"
#include "GameFramework/CharacterMovementComponent.h"

ABearBossBase::ABearBossBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void ABearBossBase::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	bIsGrounded = true;
	bIsJumping = false;

	OnLandedDelegate.Broadcast();
}

bool ABearBossBase::GetIsGrounded()
{
	bIsGrounded = !GetCharacterMovement()->IsFalling();
	return bIsGrounded;
}

bool ABearBossBase::GetIsJumping()
{
	bIsJumping = GetCharacterMovement()->IsFalling();
	return bIsJumping;
}

void ABearBossBase::SetIsGrounded(bool IsGrounded)
{
	bIsGrounded = IsGrounded;
}

void ABearBossBase::SetIsJumping(bool IsJumping)
{
	bIsJumping = IsJumping;
}

bool ABearBossBase::GetIsCharge()
{
	return bIsCharge;
}

void ABearBossBase::SetIsCharge(bool IsCharge)
{
	bIsCharge = IsCharge;
}

void ABearBossBase::ChargeStart()
{
	bIsCharge = true;
	
}

void ABearBossBase::ChargeEnd()
{
	bIsCharge = false;
	GetCharacterMovement()->GroundFriction = 8.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2048.f;
	GetCharacterMovement()->StopMovementImmediately();
}
