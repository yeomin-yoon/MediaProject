#include "LockOnLyraBridgeComponent.h"

#include "Camera/LyraCameraMode.h"
#include "Camera/LyraCameraComponent.h"
#include "Character/LyraPawnData.h"
#include "Character/LyraPawnExtensionComponent.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/Pawn.h"
#include "Input/LyraInputConfig.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "Components/InputComponent.h"
#include "InputCoreTypes.h"
#include "LockOnComponent.h"
#include "LockOnLyraCameraMode.h"
#include "LockOnSettings.h"
#include "LyraGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LockOnLyraBridgeComponent)

namespace LockOnLyraBridgeComponentPrivate
{
	static double GetLargestAxisMagnitude(const FVector2D& Value)
	{
		return FMath::Max(FMath::Abs(Value.X), FMath::Abs(Value.Y));
	}

	static void AccumulateMouseAxis(double AxisInput, double DeadZone, double& PendingAxisInput)
	{
		if (FMath::Abs(AxisInput) <= DeadZone)
		{
			return;
		}

		if ((PendingAxisInput != 0.0f) && (FMath::Sign(PendingAxisInput) != FMath::Sign(AxisInput)))
		{
			PendingAxisInput = 0.0f;
		}

		PendingAxisInput += AxisInput;
	}

	static bool TrySelectCycleDirection(const FVector2D& PendingInput, double Threshold, ELockOnCycleDirection& OutCycleDirection)
	{
		const double HorizontalMagnitude = FMath::Abs(PendingInput.X);
		const double VerticalMagnitude = FMath::Abs(PendingInput.Y);
		if ((HorizontalMagnitude < Threshold) && (VerticalMagnitude < Threshold))
		{
			return false;
		}

		if (HorizontalMagnitude >= VerticalMagnitude)
		{
			OutCycleDirection = (PendingInput.X >= 0.0f) ? ELockOnCycleDirection::Right : ELockOnCycleDirection::Left;
			return true;
		}

		OutCycleDirection = (PendingInput.Y >= 0.0f) ? ELockOnCycleDirection::Up : ELockOnCycleDirection::Down;
		return true;
	}

	static ULyraCameraComponent* FindLyraCameraComponentByName(const AActor* Actor)
	{
		if (!Actor)
		{
			return nullptr;
		}

		TInlineComponentArray<UActorComponent*> Components(Actor);
		for (UActorComponent* Component : Components)
		{
			if (Component && Component->GetClass() && (Component->GetClass()->GetName() == TEXT("LyraCameraComponent")))
			{
				return static_cast<ULyraCameraComponent*>(Component);
			}
		}

		return nullptr;
	}

	static const UInputAction* FindNativeInputAction(const ULyraInputConfig* InputConfig, const FGameplayTag& InputTag)
	{
		if (!InputConfig)
		{
			return nullptr;
		}

		for (const FLyraInputAction& Action : InputConfig->NativeInputActions)
		{
			if (Action.InputAction && (Action.InputTag == InputTag))
			{
				return Action.InputAction;
			}
		}

		return nullptr;
	}
}

ULockOnLyraBridgeComponent::ULockOnLyraBridgeComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	LockOnCameraModeClass = ULockOnLyraCameraMode::StaticClass();
}

void ULockOnLyraBridgeComponent::BeginPlay()
{
	Super::BeginPlay();
	CachedLockOnComponent = GetOwner() ? GetOwner()->FindComponentByClass<ULockOnComponent>() : nullptr;
	EnsureDelegateChain();
}

void ULockOnLyraBridgeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ReleaseDelegateChain();
	Super::EndPlay(EndPlayReason);
}

void ULockOnLyraBridgeComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!CachedLockOnComponent && GetOwner())
	{
		CachedLockOnComponent = GetOwner()->FindComponentByClass<ULockOnComponent>();
	}

	if (ShouldManageCamera())
	{
		EnsureDelegateChain();
		TryBindInput();
		ProcessMouseCycle(DeltaTime);
	}
	else
	{
		PendingMouseCycleInput = FVector2D::ZeroVector;
		MouseCycleCooldownRemaining = 0.0f;
		MouseCycleIdleSeconds = 0.0f;
		ReleaseDelegateChain();
	}
}

void ULockOnLyraBridgeComponent::EnsureDelegateChain()
{
	if (!ShouldManageCamera() || !GetOwner())
	{
		return;
	}

	ULyraCameraComponent* CameraComponent = LockOnLyraBridgeComponentPrivate::FindLyraCameraComponentByName(GetOwner());
	if (!CameraComponent)
	{
		return;
	}

	if (CachedCameraComponent.Get() != CameraComponent)
	{
		ReleaseDelegateChain();
		CachedCameraComponent = CameraComponent;
	}

	if (CameraComponent->DetermineCameraModeDelegate.IsBoundToObject(this))
	{
		return;
	}

	BaseCameraModeDelegate = CameraComponent->DetermineCameraModeDelegate;
	CameraComponent->DetermineCameraModeDelegate.BindUObject(this, &ThisClass::DetermineCameraModeFromBridge);

	if (CachedLockOnComponent && bHasBoundDelegateOnce)
	{
		CachedLockOnComponent->NotifyDelegateChainRecovered();
	}

	bHasBoundDelegateOnce = true;
}

void ULockOnLyraBridgeComponent::ReleaseDelegateChain()
{
	if (ULyraCameraComponent* CameraComponent = CachedCameraComponent.Get())
	{
		if (CameraComponent->DetermineCameraModeDelegate.IsBoundToObject(this))
		{
			if (BaseCameraModeDelegate.IsBound())
			{
				CameraComponent->DetermineCameraModeDelegate = BaseCameraModeDelegate;
			}
			else
			{
				CameraComponent->DetermineCameraModeDelegate.Unbind();
			}
		}
	}

	CachedCameraComponent.Reset();
	BaseCameraModeDelegate.Unbind();
}

void ULockOnLyraBridgeComponent::TryBindInput()
{
	const ULockOnSettings* Settings = ULockOnSettings::Get();
	if (!GetOwner())
	{
		return;
	}

	APawn* Pawn = Cast<APawn>(GetOwner());
	if (!Pawn || !Pawn->IsLocallyControlled())
	{
		return;
	}

	UInputComponent* InputComponent = Pawn->InputComponent;
	if (!InputComponent || (BoundInputComponent.Get() == InputComponent))
	{
		return;
	}

	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);
	if (!EnhancedInputComponent)
	{
		return;
	}

	const ULyraPawnExtensionComponent* PawnExtension = ULyraPawnExtensionComponent::FindPawnExtensionComponent(Pawn);
	const ULyraPawnData* PawnData = PawnExtension ? PawnExtension->GetPawnData<ULyraPawnData>() : nullptr;
	const ULyraInputConfig* InputConfig = PawnData ? PawnData->InputConfig : nullptr;
	if (!InputConfig)
	{
		return;
	}

	if (const UInputAction* LookMouseAction = LockOnLyraBridgeComponentPrivate::FindNativeInputAction(InputConfig, LyraGameplayTags::InputTag_Look_Mouse))
	{
		EnhancedInputComponent->BindAction(LookMouseAction, ETriggerEvent::Triggered, this, &ThisClass::HandleLookMouse);
	}
	else
	{
		return;
	}

	if (Settings->bBindMiddleMouseToggle)
	{
		InputComponent->BindKey(EKeys::MiddleMouseButton, IE_Pressed, this, &ThisClass::HandleToggleLockPressed);
	}

	BoundInputComponent = InputComponent;
}

void ULockOnLyraBridgeComponent::ProcessMouseCycle(float DeltaTime)
{
	const ULockOnSettings* Settings = ULockOnSettings::Get();
	if (MouseCycleCooldownRemaining > 0.0f)
	{
		MouseCycleCooldownRemaining = FMath::Max(0.0f, MouseCycleCooldownRemaining - DeltaTime);
	}

	if (!Settings->bEnableMouseMoveCycle || !CachedLockOnComponent || !CachedLockOnComponent->IsLockActive())
	{
		PendingMouseCycleInput = FVector2D::ZeroVector;
		MouseCycleIdleSeconds = 0.0f;
		return;
	}

	MouseCycleIdleSeconds += DeltaTime;
	if ((MouseCycleIdleSeconds >= Settings->MouseCycleIdleResetSeconds) && (LockOnLyraBridgeComponentPrivate::GetLargestAxisMagnitude(PendingMouseCycleInput) < Settings->MouseCycleThreshold))
	{
		PendingMouseCycleInput = FVector2D::ZeroVector;
	}

	if (MouseCycleCooldownRemaining > 0.0f)
	{
		return;
	}

	ELockOnCycleDirection CycleDirection = ELockOnCycleDirection::Right;
	if (!LockOnLyraBridgeComponentPrivate::TrySelectCycleDirection(PendingMouseCycleInput, Settings->MouseCycleThreshold, CycleDirection))
	{
		return;
	}

	switch (CycleDirection)
	{
	case ELockOnCycleDirection::Left:
		CachedLockOnComponent->RequestCycleLeft();
		PendingMouseCycleInput.X = FMath::Min(0.0f, PendingMouseCycleInput.X + Settings->MouseCycleThreshold);
		PendingMouseCycleInput.Y = 0.0f;
		break;

	case ELockOnCycleDirection::Right:
		CachedLockOnComponent->RequestCycleRight();
		PendingMouseCycleInput.X = FMath::Max(0.0f, PendingMouseCycleInput.X - Settings->MouseCycleThreshold);
		PendingMouseCycleInput.Y = 0.0f;
		break;

	case ELockOnCycleDirection::Up:
		CachedLockOnComponent->RequestCycleUp();
		PendingMouseCycleInput.Y = FMath::Max(0.0f, PendingMouseCycleInput.Y - Settings->MouseCycleThreshold);
		PendingMouseCycleInput.X = 0.0f;
		break;

	case ELockOnCycleDirection::Down:
		CachedLockOnComponent->RequestCycleDown();
		PendingMouseCycleInput.Y = FMath::Min(0.0f, PendingMouseCycleInput.Y + Settings->MouseCycleThreshold);
		PendingMouseCycleInput.X = 0.0f;
		break;

	default:
		return;
	}

	MouseCycleCooldownRemaining = Settings->MouseCycleCooldownSeconds;
}

bool ULockOnLyraBridgeComponent::ShouldManageCamera() const
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	return Pawn && Pawn->IsLocallyControlled();
}

TSubclassOf<ULyraCameraMode> ULockOnLyraBridgeComponent::DetermineCameraModeFromBridge() const
{
	if (CachedLockOnComponent && CachedLockOnComponent->IsLockActive())
	{
		return LockOnCameraModeClass;
	}

	return BaseCameraModeDelegate.IsBound() ? BaseCameraModeDelegate.Execute() : nullptr;
}

void ULockOnLyraBridgeComponent::HandleToggleLockPressed()
{
	if (CachedLockOnComponent)
	{
		CachedLockOnComponent->RequestToggleLock();
	}
}

void ULockOnLyraBridgeComponent::HandleLookMouse(const FInputActionValue& InputActionValue)
{
	const ULockOnSettings* Settings = ULockOnSettings::Get();
	if (!Settings->bEnableMouseMoveCycle || !CachedLockOnComponent || !CachedLockOnComponent->IsLockActive())
	{
		PendingMouseCycleInput = FVector2D::ZeroVector;
		MouseCycleIdleSeconds = 0.0f;
		return;
	}

	const FVector2D LookValue = InputActionValue.Get<FVector2D>();
	const bool bHasHorizontalInput = (FMath::Abs(LookValue.X) > Settings->MouseCycleDeadZone);
	const bool bHasVerticalInput = (FMath::Abs(LookValue.Y) > Settings->MouseCycleDeadZone);
	if (!bHasHorizontalInput && !bHasVerticalInput)
	{
		return;
	}

	LockOnLyraBridgeComponentPrivate::AccumulateMouseAxis(LookValue.X, Settings->MouseCycleDeadZone, PendingMouseCycleInput.X);
	LockOnLyraBridgeComponentPrivate::AccumulateMouseAxis(LookValue.Y, Settings->MouseCycleDeadZone, PendingMouseCycleInput.Y);
	MouseCycleIdleSeconds = 0.0f;
}
