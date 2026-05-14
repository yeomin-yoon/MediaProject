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
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Character/LyraHealthComponent.h"
#include "GameModes/LyraUserFacingExperienceDefinition.h"
#include "GameplayTagContainer.h"
#include "Lobby/LyraLobbyLoadoutFunctionLibrary.h"
#include "Lobby/LyraLobbyPlayerStateComponent.h"
#include "Player/LyraPlayerController.h"
#include "UObject/UObjectIterator.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraHUD)

DEFINE_LOG_CATEGORY_STATIC(LogLyraActionCombatLobbyUI, Log, All);

class AActor;
class UWorld;

namespace ActionCombatLobbyUI
{
	static const FName HeadSlotName(TEXT("Cosmetic.Slot.Head"));
	static const FName FaceSlotName(TEXT("Cosmetic.Slot.Face"));
	static const FName BackSlotName(TEXT("Cosmetic.Slot.Back"));

	static FText GetSlotDisplayName(FName SlotTagName)
	{
		if (SlotTagName == HeadSlotName)
		{
			return FText::FromString(TEXT("Head"));
		}

		if (SlotTagName == FaceSlotName)
		{
			return FText::FromString(TEXT("Face"));
		}

		if (SlotTagName == BackSlotName)
		{
			return FText::FromString(TEXT("Back"));
		}

		return FText::FromName(SlotTagName);
	}

	static FSlateColor GetSlotButtonColor(bool bSelected)
	{
		return bSelected
			? FSlateColor(FLinearColor(0.75f, 0.45f, 0.16f, 1.0f))
			: FSlateColor(FLinearColor(0.18f, 0.19f, 0.20f, 1.0f));
	}

	static FSlateColor GetItemButtonColor(bool bEnabled)
	{
		return bEnabled
			? FSlateColor(FLinearColor(0.25f, 0.27f, 0.28f, 1.0f))
			: FSlateColor(FLinearColor(0.10f, 0.11f, 0.12f, 1.0f));
	}
}

//////////////////////////////////////////////////////////////////////
// ALyraHUD

ALyraHUD::ALyraHUD(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bStartWithTickEnabled = false;
	ActionCombatLobbySelectedSlotName = ActionCombatLobbyUI::HeadSlotName;
	ActionCombatLobbyStatusText = FText::FromString(TEXT("Select a cosmetic, then press Play."));
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
	CreateActionCombatLobbyUI();
}

void ALyraHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UGameFrameworkComponentManager::RemoveGameFrameworkComponentReceiver(this);
	RemoveActionCombatLobbyUI();

	Super::EndPlay(EndPlayReason);
}

void ALyraHUD::DrawHUD()
{
	Super::DrawHUD();

	if (ShouldUseActionCombatLobbyUI() && !ActionCombatLobbyOverlayWidget.IsValid())
	{
		CreateActionCombatLobbyUI();
	}

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
	return World && (World->GetMapName().Contains(TEXT("ActionCombatTest")) || World->GetMapName().Contains(TEXT("LobbyTest")));
}

bool ALyraHUD::ShouldUseActionCombatLobbyUI() const
{
	const UWorld* World = GetWorld();
	return World && World->GetMapName().Contains(TEXT("LobbyTest"));
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

void ALyraHUD::CreateActionCombatLobbyUI()
{
	if (!ShouldUseActionCombatLobbyUI() || ActionCombatLobbyOverlayWidget.IsValid() || !PlayerOwner || !PlayerOwner->IsLocalController())
	{
		return;
	}

	ULocalPlayer* LocalPlayer = PlayerOwner->GetLocalPlayer();
	UGameViewportClient* GameViewport = LocalPlayer ? LocalPlayer->ViewportClient : nullptr;
	if (!LocalPlayer || !GameViewport)
	{
		UE_LOG(LogLyraActionCombatLobbyUI, Warning, TEXT("Lobby UI skipped: LocalPlayer=%s ViewportClient=%s PlayerOwner=%s"),
			*GetNameSafe(LocalPlayer),
			*GetNameSafe(GameViewport),
			*GetNameSafe(PlayerOwner));
		return;
	}

	ActionCombatLobbyOverlayWidget = BuildActionCombatLobbyWidget();

	GameViewport->AddViewportWidgetForPlayer(LocalPlayer, ActionCombatLobbyOverlayWidget.ToSharedRef(), 1000);
	UE_LOG(LogLyraActionCombatLobbyUI, Log, TEXT("Lobby UI added for player %s on map %s."),
		*GetNameSafe(PlayerOwner),
		GetWorld() ? *GetWorld()->GetMapName() : TEXT("<NoWorld>"));

	PlayerOwner->bShowMouseCursor = true;
	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(ActionCombatLobbyOverlayWidget);
	InputMode.SetHideCursorDuringCapture(false);
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PlayerOwner->SetInputMode(InputMode);

	SubmitActionCombatLobbyDefaultNoneIfNeeded();
}

void ALyraHUD::RemoveActionCombatLobbyUI()
{
	if (!ActionCombatLobbyOverlayWidget.IsValid())
	{
		return;
	}

	ULocalPlayer* LocalPlayer = PlayerOwner ? PlayerOwner->GetLocalPlayer() : nullptr;
	UGameViewportClient* GameViewport = LocalPlayer ? LocalPlayer->ViewportClient : nullptr;
	if (LocalPlayer && GameViewport)
	{
		GameViewport->RemoveViewportWidgetForPlayer(LocalPlayer, ActionCombatLobbyOverlayWidget.ToSharedRef());
	}

	ActionCombatLobbyOverlayWidget.Reset();
}

void ALyraHUD::SubmitActionCombatLobbyDefaultNoneIfNeeded()
{
	if (!ShouldUseActionCombatLobbyUI() || bActionCombatLobbyDefaultSelectionSubmitted || bActionCombatLobbyUserMadeSelection)
	{
		return;
	}

	if (SetActionCombatLobbyHeadAccessory(FPrimaryAssetId(), FText::FromString(TEXT("Head cosmetic cleared."))))
	{
		bActionCombatLobbyDefaultSelectionSubmitted = true;
		ActionCombatLobbyStatusText = FText::FromString(TEXT("Select a cosmetic, then press Play."));
	}
}

TSharedRef<SWidget> ALyraHUD::BuildActionCombatLobbyWidget()
{
	auto MakeSlotButton = [this](FName SlotTagName, const TCHAR* Label, const TCHAR* Description, FReply (ALyraHUD::*Handler)()) -> TSharedRef<SWidget>
	{
		return SNew(SButton)
			.OnClicked_UObject(this, Handler)
			.ButtonColorAndOpacity_Lambda([this, SlotTagName]()
			{
				return ActionCombatLobbyUI::GetSlotButtonColor(IsActionCombatLobbySlotSelected(SlotTagName));
			})
			.ContentPadding(FMargin(10.0f, 8.0f))
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(FText::FromString(Label))
					.ColorAndOpacity(FLinearColor::White)
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FMargin(0.0f, 2.0f, 0.0f, 0.0f))
				[
					SNew(STextBlock)
					.Text(FText::FromString(Description))
					.ColorAndOpacity(FLinearColor(0.70f, 0.70f, 0.70f, 1.0f))
				]
			];
	};

	return SNew(SOverlay)
		+ SOverlay::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Top)
		.Padding(FMargin(36.0f))
		[
			SNew(SBox)
			.WidthOverride(760.0f)
			[
				SNew(SBorder)
				.BorderBackgroundColor(FLinearColor(0.025f, 0.025f, 0.023f, 0.92f))
				.Padding(FMargin(14.0f))
				[
					SNew(SVerticalBox)

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(0.0f, 0.0f, 0.0f, 10.0f))
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("LOADOUT")))
						.ColorAndOpacity(FLinearColor::White)
					]

					+ SVerticalBox::Slot()
					.FillHeight(1.0f)
					[
						SNew(SHorizontalBox)

						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(FMargin(0.0f, 0.0f, 12.0f, 0.0f))
						[
							SNew(SBox)
							.WidthOverride(190.0f)
							[
								SNew(SBorder)
								.BorderBackgroundColor(FLinearColor(0.055f, 0.057f, 0.055f, 0.95f))
								.Padding(FMargin(10.0f))
								[
									SNew(SVerticalBox)

									+ SVerticalBox::Slot()
									.AutoHeight()
									.Padding(FMargin(0.0f, 0.0f, 0.0f, 8.0f))
									[
										SNew(STextBlock)
										.Text(FText::FromString(TEXT("SLOTS")))
										.ColorAndOpacity(FLinearColor(0.85f, 0.79f, 0.66f, 1.0f))
									]

									+ SVerticalBox::Slot()
									.AutoHeight()
									.Padding(FMargin(0.0f, 0.0f, 0.0f, 7.0f))
									[
										MakeSlotButton(ActionCombatLobbyUI::HeadSlotName, TEXT("Head"), TEXT("Hats / helmets"), &ALyraHUD::HandleLobbySelectHeadSlotClicked)
									]

									+ SVerticalBox::Slot()
									.AutoHeight()
									.Padding(FMargin(0.0f, 0.0f, 0.0f, 7.0f))
									[
										MakeSlotButton(ActionCombatLobbyUI::FaceSlotName, TEXT("Face"), TEXT("Masks / glasses"), &ALyraHUD::HandleLobbySelectFaceSlotClicked)
									]

									+ SVerticalBox::Slot()
									.AutoHeight()
									[
										MakeSlotButton(ActionCombatLobbyUI::BackSlotName, TEXT("Back"), TEXT("Capes / packs"), &ALyraHUD::HandleLobbySelectBackSlotClicked)
									]
								]
							]
						]

						+ SHorizontalBox::Slot()
						.FillWidth(1.0f)
						[
							SNew(SBorder)
							.BorderBackgroundColor(FLinearColor(0.072f, 0.073f, 0.069f, 0.95f))
							.Padding(FMargin(12.0f))
							[
								SNew(SVerticalBox)

								+ SVerticalBox::Slot()
								.AutoHeight()
								.Padding(FMargin(0.0f, 0.0f, 0.0f, 10.0f))
								[
									SNew(STextBlock)
									.Text_UObject(this, &ThisClass::GetActionCombatLobbySelectedSlotText)
									.ColorAndOpacity(FLinearColor(0.96f, 0.91f, 0.78f, 1.0f))
								]

								+ SVerticalBox::Slot()
								.AutoHeight()
								.Padding(FMargin(0.0f, 0.0f, 0.0f, 8.0f))
								[
									SNew(SButton)
									.OnClicked_UObject(this, &ThisClass::HandleLobbySelectNoneClicked)
									.ButtonColorAndOpacity(ActionCombatLobbyUI::GetItemButtonColor(true))
									.ContentPadding(FMargin(10.0f))
									[
										SNew(SHorizontalBox)

										+ SHorizontalBox::Slot()
										.AutoWidth()
										.Padding(FMargin(0.0f, 0.0f, 10.0f, 0.0f))
										[
											SNew(SBox)
											.WidthOverride(54.0f)
											.HeightOverride(54.0f)
											[
												SNew(SBorder)
												.BorderBackgroundColor(FLinearColor(0.08f, 0.08f, 0.08f, 1.0f))
												[
													SNew(STextBlock)
													.Text(FText::FromString(TEXT("-")))
													.Justification(ETextJustify::Center)
													.ColorAndOpacity(FLinearColor(0.60f, 0.60f, 0.60f, 1.0f))
												]
											]
										]

										+ SHorizontalBox::Slot()
										.FillWidth(1.0f)
										.VAlign(VAlign_Center)
										[
											SNew(SVerticalBox)

											+ SVerticalBox::Slot()
											.AutoHeight()
											[
												SNew(STextBlock)
												.Text(FText::FromString(TEXT("None")))
												.ColorAndOpacity(FLinearColor::White)
											]

											+ SVerticalBox::Slot()
											.AutoHeight()
											.Padding(FMargin(0.0f, 2.0f, 0.0f, 0.0f))
											[
												SNew(STextBlock)
												.Text(FText::FromString(TEXT("Clear the selected cosmetic slot.")))
												.ColorAndOpacity(FLinearColor(0.72f, 0.72f, 0.72f, 1.0f))
											]
										]
									]
								]

								+ SVerticalBox::Slot()
								.AutoHeight()
								.Padding(FMargin(0.0f, 0.0f, 0.0f, 8.0f))
								[
									SNew(SButton)
									.OnClicked_UObject(this, &ThisClass::HandleLobbySelectHeadCubeClicked)
									.ButtonColorAndOpacity(ActionCombatLobbyUI::GetItemButtonColor(true))
									.ContentPadding(FMargin(10.0f))
									.Visibility_Lambda([this]()
									{
										return IsActionCombatLobbySlotSelected(ActionCombatLobbyUI::HeadSlotName) ? EVisibility::Visible : EVisibility::Collapsed;
									})
									[
										SNew(SHorizontalBox)

										+ SHorizontalBox::Slot()
										.AutoWidth()
										.Padding(FMargin(0.0f, 0.0f, 10.0f, 0.0f))
										[
											SNew(SBox)
											.WidthOverride(54.0f)
											.HeightOverride(54.0f)
											[
												SNew(SBorder)
												.BorderBackgroundColor(FLinearColor(0.15f, 0.28f, 0.56f, 1.0f))
												[
													SNew(STextBlock)
													.Text(FText::FromString(TEXT("C")))
													.Justification(ETextJustify::Center)
													.ColorAndOpacity(FLinearColor::White)
												]
											]
										]

										+ SHorizontalBox::Slot()
										.FillWidth(1.0f)
										.VAlign(VAlign_Center)
										[
											SNew(SVerticalBox)

											+ SVerticalBox::Slot()
											.AutoHeight()
											[
												SNew(STextBlock)
												.Text(FText::FromString(TEXT("Head Cube")))
												.ColorAndOpacity(FLinearColor::White)
											]

											+ SVerticalBox::Slot()
											.AutoHeight()
											.Padding(FMargin(0.0f, 2.0f, 0.0f, 0.0f))
											[
												SNew(STextBlock)
												.Text(FText::FromString(TEXT("Temporary test hat asset.")))
												.ColorAndOpacity(FLinearColor(0.72f, 0.72f, 0.72f, 1.0f))
											]
										]
									]
								]

								+ SVerticalBox::Slot()
								.AutoHeight()
								[
									SNew(SBorder)
									.Visibility_Lambda([this]()
									{
										return IsActionCombatLobbySlotSelected(ActionCombatLobbyUI::HeadSlotName) ? EVisibility::Collapsed : EVisibility::Visible;
									})
									.BorderBackgroundColor(FLinearColor(0.09f, 0.09f, 0.085f, 1.0f))
									.Padding(FMargin(10.0f))
									[
										SNew(STextBlock)
										.Text(FText::FromString(TEXT("No item data is registered for this slot yet.")))
										.ColorAndOpacity(FLinearColor(0.74f, 0.74f, 0.74f, 1.0f))
									]
								]
							]
						]
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(0.0f, 12.0f, 0.0f, 0.0f))
					[
						SNew(SHorizontalBox)

						+ SHorizontalBox::Slot()
						.FillWidth(1.0f)
						.VAlign(VAlign_Center)
						.Padding(FMargin(0.0f, 0.0f, 12.0f, 0.0f))
						[
							SNew(STextBlock)
							.Text_UObject(this, &ThisClass::GetActionCombatLobbyStatusText)
							.ColorAndOpacity(FLinearColor(0.80f, 0.80f, 0.80f, 1.0f))
							.AutoWrapText(true)
						]

						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SBox)
							.WidthOverride(130.0f)
							[
								SNew(SButton)
								.OnClicked_UObject(this, &ThisClass::HandleLobbyPlayClicked)
								.ButtonColorAndOpacity(FLinearColor(0.58f, 0.32f, 0.12f, 1.0f))
								[
									SNew(STextBlock)
									.Text(FText::FromString(TEXT("Play")))
									.Justification(ETextJustify::Center)
									.ColorAndOpacity(FLinearColor::White)
								]
							]
						]
					]
				]
			]
		];
}

FReply ALyraHUD::HandleLobbySelectHeadSlotClicked()
{
	ActionCombatLobbySelectedSlotName = ActionCombatLobbyUI::HeadSlotName;
	ActionCombatLobbyStatusText = FText::FromString(TEXT("Head slot selected."));
	return FReply::Handled();
}

FReply ALyraHUD::HandleLobbySelectFaceSlotClicked()
{
	ActionCombatLobbySelectedSlotName = ActionCombatLobbyUI::FaceSlotName;
	ActionCombatLobbyStatusText = FText::FromString(TEXT("Face slot selected."));
	return FReply::Handled();
}

FReply ALyraHUD::HandleLobbySelectBackSlotClicked()
{
	ActionCombatLobbySelectedSlotName = ActionCombatLobbyUI::BackSlotName;
	ActionCombatLobbyStatusText = FText::FromString(TEXT("Back slot selected."));
	return FReply::Handled();
}

FReply ALyraHUD::HandleLobbySelectNoneClicked()
{
	bActionCombatLobbyUserMadeSelection = true;
	const FText SlotName = ActionCombatLobbyUI::GetSlotDisplayName(ActionCombatLobbySelectedSlotName);
	SetActionCombatLobbyAccessory(
		ActionCombatLobbySelectedSlotName,
		FPrimaryAssetId(),
		FText::Format(FText::FromString(TEXT("{0} cosmetic cleared.")), SlotName));
	return FReply::Handled();
}

FReply ALyraHUD::HandleLobbySelectHeadCubeClicked()
{
	bActionCombatLobbyUserMadeSelection = true;
	const FPrimaryAssetId AccessoryId = ULyraLobbyLoadoutFunctionLibrary::MakeLobbyPrimaryAssetId(
		FName(TEXT("ActionCombatAccessoryData")),
		FName(TEXT("DA_DragonKnightRuntime_TestAccessory_HeadCube")));
	SetActionCombatLobbyAccessory(ActionCombatLobbyUI::HeadSlotName, AccessoryId, FText::FromString(TEXT("Head Cube saved.")));
	return FReply::Handled();
}

FReply ALyraHUD::HandleLobbyPlayClicked()
{
	UObject* ExperienceObject = StaticLoadObject(
		ULyraUserFacingExperienceDefinition::StaticClass(),
		nullptr,
		TEXT("/Game/1dev/OS/UI/DA_LobbyPlay_ActionCombatTest.DA_LobbyPlay_ActionCombatTest"));
	const ULyraUserFacingExperienceDefinition* Experience = Cast<ULyraUserFacingExperienceDefinition>(ExperienceObject);
	if (!Experience)
	{
		ActionCombatLobbyStatusText = FText::FromString(TEXT("Target experience could not be loaded."));
		return FReply::Handled();
	}

	const int32 LocalPlayerIndex = GetActionCombatLobbyLocalPlayerIndex();
	if (ULyraLobbyLoadoutFunctionLibrary::QuickPlayLocalLobbyExperience(this, Experience, LocalPlayerIndex, ECommonSessionOnlineMode::LAN))
	{
		ActionCombatLobbyStatusText = FText::FromString(TEXT("Starting travel..."));
	}
	else
	{
		ActionCombatLobbyStatusText = FText::FromString(TEXT("Failed to start travel."));
	}

	return FReply::Handled();
}

FText ALyraHUD::GetActionCombatLobbyStatusText() const
{
	return ActionCombatLobbyStatusText;
}

FText ALyraHUD::GetActionCombatLobbySelectedSlotText() const
{
	return FText::Format(
		FText::FromString(TEXT("{0} items")),
		ActionCombatLobbyUI::GetSlotDisplayName(ActionCombatLobbySelectedSlotName));
}

bool ALyraHUD::IsActionCombatLobbySlotSelected(FName SlotTagName) const
{
	return ActionCombatLobbySelectedSlotName == SlotTagName;
}

bool ALyraHUD::SetActionCombatLobbyAccessory(FName SlotTagName, FPrimaryAssetId AccessoryId, const FText& SuccessMessage)
{
	const FGameplayTag SlotTag = FGameplayTag::RequestGameplayTag(SlotTagName, false);
	if (!SlotTag.IsValid())
	{
		ActionCombatLobbyStatusText = FText::Format(FText::FromString(TEXT("{0} tag is not registered.")), FText::FromName(SlotTagName));
		return false;
	}

	const int32 LocalPlayerIndex = GetActionCombatLobbyLocalPlayerIndex();
	ULyraLobbyPlayerStateComponent* LobbyPlayer = GetActionCombatLobbyPlayerStateComponent();

	FLyraLobbyPlayerLoadout Loadout;
	if (LobbyPlayer)
	{
		Loadout = LobbyPlayer->GetLobbyLoadout();
	}
	else
	{
		ULyraLobbyLoadoutFunctionLibrary::GetLocalLobbyLoadout(this, Loadout, LocalPlayerIndex);
	}

	Loadout = AccessoryId.IsValid()
		? ULyraLobbyLoadoutFunctionLibrary::SetAccessorySlot(Loadout, SlotTag, AccessoryId)
		: ULyraLobbyLoadoutFunctionLibrary::ClearAccessorySlot(Loadout, SlotTag);

	const bool bSavedForTravel = ULyraLobbyLoadoutFunctionLibrary::SaveLocalLobbyLoadoutForTravel(this, Loadout, LocalPlayerIndex);
	bool bSubmittedToLobby = false;
	if (ALyraPlayerController* LyraPlayerController = Cast<ALyraPlayerController>(PlayerOwner))
	{
		bSubmittedToLobby = LyraPlayerController->SubmitLocalLobbyLoadout(Loadout);
	}
	else if (LobbyPlayer)
	{
		LobbyPlayer->SubmitLobbyLoadout(Loadout);
		bSubmittedToLobby = true;
	}
	else
	{
		bSubmittedToLobby = ULyraLobbyLoadoutFunctionLibrary::SubmitLocalLobbyLoadout(this, Loadout, LocalPlayerIndex);
	}

	UE_LOG(LogLyraActionCombatLobbyUI, Log, TEXT("Lobby accessory changed. Player=%s LocalPlayerIndex=%d Slot=%s Accessory=%s SavedForTravel=%s SubmittedToLobby=%s Slots=%d"),
		*GetNameSafe(PlayerOwner),
		LocalPlayerIndex,
		*SlotTag.ToString(),
		*AccessoryId.ToString(),
		bSavedForTravel ? TEXT("true") : TEXT("false"),
		bSubmittedToLobby ? TEXT("true") : TEXT("false"),
		Loadout.AccessorySlots.Num());

	if (bSubmittedToLobby)
	{
		ActionCombatLobbyStatusText = SuccessMessage;
		return true;
	}

	if (bSavedForTravel)
	{
		ActionCombatLobbyStatusText = FText::Format(FText::FromString(TEXT("{0} Saved locally for travel.")), SuccessMessage);
		return true;
	}

	ActionCombatLobbyStatusText = FText::FromString(TEXT("Failed to save cosmetic selection."));
	return false;
}

bool ALyraHUD::SetActionCombatLobbyHeadAccessory(FPrimaryAssetId AccessoryId, const FText& SuccessMessage)
{
	return SetActionCombatLobbyAccessory(ActionCombatLobbyUI::HeadSlotName, AccessoryId, SuccessMessage);
}

int32 ALyraHUD::GetActionCombatLobbyLocalPlayerIndex() const
{
	const ULocalPlayer* LocalPlayer = PlayerOwner ? PlayerOwner->GetLocalPlayer() : nullptr;
	return LocalPlayer ? LocalPlayer->GetLocalPlayerIndex() : 0;
}

ULyraLobbyPlayerStateComponent* ALyraHUD::GetActionCombatLobbyPlayerStateComponent() const
{
	const APlayerState* PlayerState = PlayerOwner ? PlayerOwner->PlayerState : nullptr;
	return PlayerState ? PlayerState->FindComponentByClass<ULyraLobbyPlayerStateComponent>() : nullptr;
}

