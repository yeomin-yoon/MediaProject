// Copyright Epic Games, Inc. All Rights Reserved.

#include "LyraHUD.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Async/TaskGraphInterfaces.h"
#include "CanvasItem.h"
#include "Components/GameFrameworkComponentManager.h"
#include "Components/InputComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Font.h"
#include "GameFramework/PlayerController.h"
#include "Character/LyraHealthComponent.h"
#include "UObject/UObjectIterator.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraHUD)

class AActor;
class UWorld;

//////////////////////////////////////////////////////////////////////
// ALyraHUD

ALyraHUD::ALyraHUD(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bStartWithTickEnabled = false;
}

void ALyraHUD::PreInitializeComponents()
{
	Super::PreInitializeComponents();

	UGameFrameworkComponentManager::AddGameFrameworkComponentReceiver(this);
}

void ALyraHUD::BeginPlay()
{
	UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(this, UGameFrameworkComponentManager::NAME_GameActorReady);

	Super::BeginPlay();
	BindActionCombatFallbackInput();
}

void ALyraHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UGameFrameworkComponentManager::RemoveGameFrameworkComponentReceiver(this);

	Super::EndPlay(EndPlayReason);
}

void ALyraHUD::DrawHUD()
{
	Super::DrawHUD();

	if (ShouldUseActionCombatFallbackHUD())
	{
		DrawActionCombatFallbackHUD();
	}
}

void ALyraHUD::GetDebugActorList(TArray<AActor*>& InOutList)
{
	UWorld* World = GetWorld();

	Super::GetDebugActorList(InOutList);

	// Add all actors with an ability system component.
	for (TObjectIterator<UAbilitySystemComponent> It; It; ++It)
	{
		if (UAbilitySystemComponent* ASC = *It)
		{
			if (!ASC->HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))
			{
				AActor* AvatarActor = ASC->GetAvatarActor();
				AActor* OwnerActor = ASC->GetOwnerActor();

				if (AvatarActor && UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(AvatarActor))
				{
					AddActorToDebugList(AvatarActor, InOutList, World);
				}
				else if (OwnerActor && UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwnerActor))
				{
					AddActorToDebugList(OwnerActor, InOutList, World);
				}
			}
		}
	}
}

bool ALyraHUD::ShouldUseActionCombatFallbackHUD() const
{
	const UWorld* World = GetWorld();
	return World && World->GetMapName().Contains(TEXT("ActionCombatTest"));
}

void ALyraHUD::BindActionCombatFallbackInput()
{
	if (!ShouldUseActionCombatFallbackHUD() || !PlayerOwner)
	{
		return;
	}

	EnableInput(PlayerOwner);
	if (!InputComponent)
	{
		return;
	}

	InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &ThisClass::HandleFallbackEscapePressed);
	InputComponent->BindKey(EKeys::Q, IE_Pressed, this, &ThisClass::HandleFallbackQuitPressed);
	InputComponent->BindKey(EKeys::Enter, IE_Pressed, this, &ThisClass::HandleFallbackQuitPressed);
	InputComponent->BindKey(EKeys::R, IE_Pressed, this, &ThisClass::HandleFallbackResumePressed);
}

void ALyraHUD::HandleFallbackEscapePressed()
{
	if (!ShouldUseActionCombatFallbackHUD())
	{
		return;
	}

	bActionCombatFallbackMenuOpen = !bActionCombatFallbackMenuOpen;
}

void ALyraHUD::HandleFallbackQuitPressed()
{
	if (!bActionCombatFallbackMenuOpen || !PlayerOwner)
	{
		return;
	}

	PlayerOwner->ConsoleCommand(TEXT("quit"));
}

void ALyraHUD::HandleFallbackResumePressed()
{
	if (bActionCombatFallbackMenuOpen)
	{
		bActionCombatFallbackMenuOpen = false;
	}
}

void ALyraHUD::DrawActionCombatFallbackHUD()
{
	if (!Canvas || !PlayerOwner)
	{
		return;
	}

	APawn* Pawn = PlayerOwner->GetPawn();
	if (!Pawn)
	{
		return;
	}

	constexpr float LeftMargin = 48.0f;
	const float BottomY = Canvas->ClipY - 96.0f;

	if (const ULyraHealthComponent* HealthComponent = ULyraHealthComponent::FindHealthComponent(Pawn))
	{
		DrawActionCombatBar(TEXT("HP"), HealthComponent->GetHealth(), HealthComponent->GetMaxHealth(), FVector2D(LeftMargin, BottomY - 38.0f), FLinearColor(0.82f, 0.14f, 0.12f, 1.0f));
	}

	float Stamina = 0.0f;
	float MaxStamina = 0.0f;
	if (TryGetActionCombatStamina(Stamina, MaxStamina))
	{
		DrawActionCombatBar(TEXT("ST"), Stamina, MaxStamina, FVector2D(LeftMargin, BottomY), FLinearColor(0.20f, 0.72f, 0.32f, 1.0f));
	}

	if (bActionCombatFallbackMenuOpen)
	{
		const FVector2D Center(Canvas->ClipX * 0.5f, Canvas->ClipY * 0.5f);
		const FVector2D BoxSize(360.0f, 150.0f);
		FCanvasTileItem Backdrop(Center - BoxSize * 0.5f, BoxSize, FLinearColor(0.02f, 0.025f, 0.03f, 0.88f));
		Backdrop.BlendMode = SE_BLEND_Translucent;
		Canvas->DrawItem(Backdrop);

		FCanvasTextItem Title(Center + FVector2D(-118.0f, -50.0f), FText::FromString(TEXT("PAUSED")), GEngine->GetLargeFont(), FLinearColor::White);
		Title.EnableShadow(FLinearColor::Black);
		Canvas->DrawItem(Title);

		FCanvasTextItem Quit(Center + FVector2D(-118.0f, 2.0f), FText::FromString(TEXT("Q / Enter  Quit")), GEngine->GetMediumFont(), FLinearColor(0.95f, 0.82f, 0.62f, 1.0f));
		Quit.EnableShadow(FLinearColor::Black);
		Canvas->DrawItem(Quit);

		FCanvasTextItem Resume(Center + FVector2D(-118.0f, 34.0f), FText::FromString(TEXT("Esc / R  Resume")), GEngine->GetMediumFont(), FLinearColor(0.78f, 0.82f, 0.86f, 1.0f));
		Resume.EnableShadow(FLinearColor::Black);
		Canvas->DrawItem(Resume);
	}
}

void ALyraHUD::DrawActionCombatBar(const FString& Label, float CurrentValue, float MaxValue, const FVector2D& Position, const FLinearColor& FillColor)
{
	if (!Canvas || MaxValue <= 0.0f)
	{
		return;
	}

	const FVector2D BarSize(300.0f, 18.0f);
	const float NormalizedValue = FMath::Clamp(CurrentValue / MaxValue, 0.0f, 1.0f);

	FCanvasTextItem LabelItem(Position + FVector2D(0.0f, -2.0f), FText::FromString(Label), GEngine->GetSmallFont(), FLinearColor(0.90f, 0.90f, 0.90f, 1.0f));
	LabelItem.EnableShadow(FLinearColor::Black);
	Canvas->DrawItem(LabelItem);

	FCanvasTileItem Backdrop(Position + FVector2D(34.0f, 0.0f), BarSize, FLinearColor(0.015f, 0.018f, 0.022f, 0.82f));
	Backdrop.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(Backdrop);

	FCanvasTileItem Fill(Position + FVector2D(36.0f, 2.0f), FVector2D((BarSize.X - 4.0f) * NormalizedValue, BarSize.Y - 4.0f), FillColor);
	Fill.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(Fill);

	const FString ValueText = FString::Printf(TEXT("%.0f / %.0f"), CurrentValue, MaxValue);
	FCanvasTextItem ValueItem(Position + FVector2D(BarSize.X + 46.0f, -2.0f), FText::FromString(ValueText), GEngine->GetSmallFont(), FLinearColor(0.90f, 0.90f, 0.90f, 1.0f));
	ValueItem.EnableShadow(FLinearColor::Black);
	Canvas->DrawItem(ValueItem);
}

bool ALyraHUD::TryGetActionCombatStamina(float& OutStamina, float& OutMaxStamina) const
{
	OutStamina = 0.0f;
	OutMaxStamina = 0.0f;

	const APawn* Pawn = PlayerOwner ? PlayerOwner->GetPawn() : nullptr;
	UAbilitySystemComponent* AbilitySystemComponent = Pawn ? UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Pawn) : nullptr;
	if (!AbilitySystemComponent)
	{
		return false;
	}

	UClass* StaminaSetClass = FindObject<UClass>(nullptr, TEXT("/Script/ActionCombatLyraBridge.ActionCombatStaminaSet"));
	if (!StaminaSetClass)
	{
		return false;
	}

	FProperty* StaminaProperty = FindFProperty<FProperty>(StaminaSetClass, TEXT("Stamina"));
	FProperty* MaxStaminaProperty = FindFProperty<FProperty>(StaminaSetClass, TEXT("MaxStamina"));
	if (!StaminaProperty || !MaxStaminaProperty)
	{
		return false;
	}

	const FGameplayAttribute StaminaAttribute(StaminaProperty);
	const FGameplayAttribute MaxStaminaAttribute(MaxStaminaProperty);
	if (!AbilitySystemComponent->HasAttributeSetForAttribute(StaminaAttribute) || !AbilitySystemComponent->HasAttributeSetForAttribute(MaxStaminaAttribute))
	{
		return false;
	}

	OutStamina = AbilitySystemComponent->GetNumericAttribute(StaminaAttribute);
	OutMaxStamina = AbilitySystemComponent->GetNumericAttribute(MaxStaminaAttribute);
	return OutMaxStamina > 0.0f;
}

