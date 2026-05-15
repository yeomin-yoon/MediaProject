#pragma once

#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "Lobby/LyraLobbyTypes.h"

#include "ActionCombatLobbyTestLoadoutWidget.generated.h"

class UActionCombatAccessoryData;
class UButton;
class UHorizontalBox;
class UImage;
class UTextBlock;
class UTexture2D;
class UVerticalBox;

USTRUCT(BlueprintType)
struct ACTIONCOMBATLYRABRIDGE_API FActionCombatLobbyCosmeticSlot
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lobby Cosmetics")
    FText Label;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lobby Cosmetics", meta = (Categories = "Cosmetic.Slot"))
    FGameplayTag SlotTag;
};

USTRUCT(BlueprintType)
struct ACTIONCOMBATLYRABRIDGE_API FActionCombatLobbyCosmeticOption
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lobby Cosmetics")
    FText Label;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lobby Cosmetics", meta = (Categories = "Cosmetic.Slot"))
    FGameplayTag SlotTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lobby Cosmetics")
    bool bClearSlot = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lobby Cosmetics")
    TSoftObjectPtr<UActionCombatAccessoryData> AccessoryData;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lobby Cosmetics")
    TSoftObjectPtr<UTexture2D> Icon;
};

UCLASS()
class ACTIONCOMBATLYRABRIDGE_API UActionCombatLobbyTestSlotEntryWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void InitializeEntry(class UActionCombatLobbyTestLoadoutWidget* InOwnerWidget, const FActionCombatLobbyCosmeticSlot& InSlot);
    void RefreshVisual(bool bSelected);
    const FActionCombatLobbyCosmeticSlot& GetSlot() const { return SlotDefinition; }

protected:
    virtual void NativeConstruct() override;

private:
    UFUNCTION()
    void HandleClicked();

    UPROPERTY(Transient)
    TObjectPtr<UButton> SlotButton = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> LabelText = nullptr;

    TWeakObjectPtr<class UActionCombatLobbyTestLoadoutWidget> OwnerWidget;
    FActionCombatLobbyCosmeticSlot SlotDefinition;
    bool bPendingBuild = false;
};

UCLASS()
class ACTIONCOMBATLYRABRIDGE_API UActionCombatLobbyTestOptionEntryWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void InitializeEntry(class UActionCombatLobbyTestLoadoutWidget* InOwnerWidget, const FActionCombatLobbyCosmeticOption& InOption);
    void RefreshVisual(bool bSelected);
    const FActionCombatLobbyCosmeticOption& GetOption() const { return Option; }

protected:
    virtual void NativeConstruct() override;

private:
    UFUNCTION()
    void HandleClicked();

    void BuildButtonContent();

    UPROPERTY(Transient)
    TObjectPtr<UButton> OptionButton = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> LabelText = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UImage> IconImage = nullptr;

    TWeakObjectPtr<class UActionCombatLobbyTestLoadoutWidget> OwnerWidget;
    FActionCombatLobbyCosmeticOption Option;
    bool bPendingBuild = false;
};

UCLASS()
class ACTIONCOMBATLYRABRIDGE_API UActionCombatLobbyTestLoadoutWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void ConfigureLobbyUi(const TArray<FActionCombatLobbyCosmeticSlot>& InSlots, const TArray<FActionCombatLobbyCosmeticOption>& InOptions, const FSoftObjectPath& InTargetExperiencePath, int32 InLocalPlayerIndex, bool bInClearSlotsOnOpen);
    void HandleSlotChosen(const FActionCombatLobbyCosmeticSlot& ChosenSlot);
    void HandleOptionChosen(const FActionCombatLobbyCosmeticOption& Option);

protected:
    virtual void NativeConstruct() override;

private:
    void BuildUiIfNeeded();
    void RebuildSlots();
    void RebuildOptions();
    void RefreshSelectionState();
    void SubmitDefaultNoneIfNeeded();
    void SetStatusMessage(const FText& Message);
    FText GetSlotTitle(const FGameplayTag& SlotTag) const;
    FText GetSelectedSlotTitle() const;
    void EnsureSlotsFromOptions();

    UFUNCTION()
    void HandlePlayClicked();

    UFUNCTION()
    void RetryRefreshState();

    UPROPERTY(Transient)
    TObjectPtr<UVerticalBox> RootVerticalBox = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UHorizontalBox> SlotsHorizontalBox = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UVerticalBox> OptionsVerticalBox = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> SelectedSlotText = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> StatusText = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UButton> PlayButton = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> PlayButtonText = nullptr;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UActionCombatLobbyTestSlotEntryWidget>> SlotEntries;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UActionCombatLobbyTestOptionEntryWidget>> OptionEntries;

    TArray<FActionCombatLobbyCosmeticSlot> CosmeticSlots;
    TArray<FActionCombatLobbyCosmeticOption> CosmeticOptions;
    FGameplayTag SelectedSlotTag;
    FLyraLobbyPlayerLoadout LastKnownLoadout;
    FSoftObjectPath TargetExperiencePath;
    int32 LocalPlayerIndex = 0;
    bool bUiBuilt = false;
    bool bClearSlotsOnOpen = true;
    bool bDefaultNoneSubmitted = false;
    bool bHasLastKnownLoadout = false;
    int32 RefreshRetryCount = 0;
    FTimerHandle RetryRefreshTimerHandle;
};
