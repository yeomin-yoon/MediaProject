// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/AssetManagerTypes.h"
#include "GameFramework/HUD.h"
#include "Input/Reply.h"
#include "Templates/SharedPointer.h"

#include "LyraHUD.generated.h"

namespace EEndPlayReason { enum Type : int; }

class AActor;
class UObject;
class SWidget;
class ULyraLobbyPlayerStateComponent;

/**
 * ALyraHUD
 *
 *  Note that you typically do not need to extend or modify this class, instead you would
 *  use an "Add Widget" action in your experience to add a HUD layout and widgets to it
 * 
 *  This class exists primarily for debug rendering
 */
UCLASS(Config = Game)
class ALyraHUD : public AHUD
{
	GENERATED_BODY()

public:
	ALyraHUD(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	//~UObject interface
	virtual void PreInitializeComponents() override;
	//~End of UObject interface

	//~AActor interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~End of AActor interface

	//~AHUD interface
	virtual void DrawHUD() override;
	virtual void GetDebugActorList(TArray<AActor*>& InOutList) override;
	//~End of AHUD interface

private:
	bool ShouldUseActionCombatFallbackHUD() const;
	bool ShouldUseActionCombatLobbyUI() const;
	void BindActionCombatFallbackInput();
	void HandleFallbackEscapePressed();
	void HandleFallbackQuitPressed();
	void HandleFallbackResumePressed();
	void DrawActionCombatFallbackHUD();
	void DrawActionCombatBar(const FString& Label, float CurrentValue, float MaxValue, const FVector2D& Position, const FLinearColor& FillColor);
	bool TryGetActionCombatStamina(float& OutStamina, float& OutMaxStamina) const;
	void CreateActionCombatLobbyUI();
	void RemoveActionCombatLobbyUI();
	void SubmitActionCombatLobbyDefaultNoneIfNeeded();
	TSharedRef<SWidget> BuildActionCombatLobbyWidget();
	FReply HandleLobbySelectHeadSlotClicked();
	FReply HandleLobbySelectFaceSlotClicked();
	FReply HandleLobbySelectBackSlotClicked();
	FReply HandleLobbySelectNoneClicked();
	FReply HandleLobbySelectHeadCubeClicked();
	FReply HandleLobbyPlayClicked();
	FText GetActionCombatLobbyStatusText() const;
	FText GetActionCombatLobbySelectedSlotText() const;
	bool IsActionCombatLobbySlotSelected(FName SlotTagName) const;
	bool SetActionCombatLobbyAccessory(FName SlotTagName, FPrimaryAssetId AccessoryId, const FText& SuccessMessage);
	bool SetActionCombatLobbyHeadAccessory(FPrimaryAssetId AccessoryId, const FText& SuccessMessage);
	int32 GetActionCombatLobbyLocalPlayerIndex() const;
	ULyraLobbyPlayerStateComponent* GetActionCombatLobbyPlayerStateComponent() const;

	bool bActionCombatFallbackMenuOpen = false;
	bool bActionCombatLobbyDefaultSelectionSubmitted = false;
	bool bActionCombatLobbyUserMadeSelection = false;
	FName ActionCombatLobbySelectedSlotName;
	TSharedPtr<SWidget> ActionCombatLobbyOverlayWidget;
	FText ActionCombatLobbyStatusText;
};
