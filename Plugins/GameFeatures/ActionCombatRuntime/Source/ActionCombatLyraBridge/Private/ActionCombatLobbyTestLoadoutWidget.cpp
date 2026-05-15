#include "ActionCombatLobbyTestLoadoutWidget.h"

#include "ActionCombatAccessoryData.h"
#include "ActionCombatLobbyCosmeticLibrary.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GameModes/LyraUserFacingExperienceDefinition.h"
#include "Lobby/LyraLobbyLoadoutFunctionLibrary.h"
#include "Logging/LogMacros.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogActionCombatLobbyTestUi, Log, All);

namespace ActionCombatLobbyUiDefaults
{
    constexpr int32 MaxRefreshRetries = 20;
    constexpr float RefreshRetryDelaySeconds = 0.25f;
}

void UActionCombatLobbyTestSlotEntryWidget::InitializeEntry(UActionCombatLobbyTestLoadoutWidget* InOwnerWidget, const FActionCombatLobbyCosmeticSlot& InSlot)
{
    OwnerWidget = InOwnerWidget;
    SlotDefinition = InSlot;
    bPendingBuild = true;
}

void UActionCombatLobbyTestSlotEntryWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (!WidgetTree)
    {
        WidgetTree = NewObject<UWidgetTree>(this, TEXT("SlotEntryWidgetTree"));
    }

    if (!WidgetTree)
    {
        UE_LOG(LogActionCombatLobbyTestUi, Warning, TEXT("Slot entry widget tree creation failed."));
        return;
    }

    if (!SlotButton)
    {
        SlotButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("SlotButton"));
        LabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SlotLabel"));
        LabelText->SetJustification(ETextJustify::Center);
        SlotButton->AddChild(LabelText);
        SlotButton->SetBackgroundColor(FLinearColor(0.08f, 0.08f, 0.08f, 0.95f));
        SlotButton->OnClicked.AddDynamic(this, &ThisClass::HandleClicked);
        WidgetTree->RootWidget = SlotButton;
    }

    if (LabelText)
    {
        LabelText->SetText(SlotDefinition.Label);
        LabelText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    }

    bPendingBuild = false;
}

void UActionCombatLobbyTestSlotEntryWidget::RefreshVisual(bool bSelected)
{
    if (SlotButton)
    {
        SlotButton->SetBackgroundColor(bSelected ? FLinearColor(0.20f, 0.31f, 0.46f, 0.98f) : FLinearColor(0.08f, 0.08f, 0.08f, 0.95f));
    }

    if (LabelText)
    {
        LabelText->SetColorAndOpacity(FSlateColor(bSelected ? FLinearColor::White : FLinearColor(0.78f, 0.78f, 0.78f, 1.0f)));
    }
}

void UActionCombatLobbyTestSlotEntryWidget::HandleClicked()
{
    if (UActionCombatLobbyTestLoadoutWidget* Owner = OwnerWidget.Get())
    {
        Owner->HandleSlotChosen(SlotDefinition);
    }
}

void UActionCombatLobbyTestOptionEntryWidget::InitializeEntry(UActionCombatLobbyTestLoadoutWidget* InOwnerWidget, const FActionCombatLobbyCosmeticOption& InOption)
{
    OwnerWidget = InOwnerWidget;
    Option = InOption;
    bPendingBuild = true;
}

void UActionCombatLobbyTestOptionEntryWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (!WidgetTree)
    {
        WidgetTree = NewObject<UWidgetTree>(this, TEXT("OptionEntryWidgetTree"));
    }

    if (!WidgetTree)
    {
        UE_LOG(LogActionCombatLobbyTestUi, Warning, TEXT("Option entry widget tree creation failed."));
        return;
    }

    if (!OptionButton)
    {
        OptionButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("OptionButton"));
        OptionButton->SetBackgroundColor(FLinearColor(0.08f, 0.08f, 0.08f, 0.95f));
        OptionButton->OnClicked.AddDynamic(this, &ThisClass::HandleClicked);
        WidgetTree->RootWidget = OptionButton;
    }

    BuildButtonContent();
    bPendingBuild = false;
}

void UActionCombatLobbyTestOptionEntryWidget::BuildButtonContent()
{
    if (!WidgetTree || !OptionButton)
    {
        return;
    }

    UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("OptionRow"));

    if (UTexture2D* IconTexture = Option.Icon.LoadSynchronous())
    {
        USizeBox* IconSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("OptionIconSize"));
        IconSize->SetWidthOverride(48.0f);
        IconSize->SetHeightOverride(48.0f);

        IconImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("OptionIcon"));
        IconImage->SetBrushFromTexture(IconTexture, true);
        IconSize->SetContent(IconImage);

        if (UHorizontalBoxSlot* IconSlot = Row->AddChildToHorizontalBox(IconSize))
        {
            IconSlot->SetPadding(FMargin(8.0f, 6.0f, 10.0f, 6.0f));
            IconSlot->SetVerticalAlignment(VAlign_Center);
        }
    }

    LabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("OptionLabel"));
    LabelText->SetText(Option.Label);
    LabelText->SetJustification(ETextJustify::Left);
    LabelText->SetColorAndOpacity(FSlateColor(FLinearColor(0.86f, 0.86f, 0.86f, 1.0f)));

    if (UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(LabelText))
    {
        LabelSlot->SetPadding(FMargin(10.0f, 10.0f, 10.0f, 10.0f));
        LabelSlot->SetVerticalAlignment(VAlign_Center);
    }

    OptionButton->SetContent(Row);
}

void UActionCombatLobbyTestOptionEntryWidget::RefreshVisual(bool bSelected)
{
    if (OptionButton)
    {
        OptionButton->SetBackgroundColor(bSelected ? FLinearColor(0.18f, 0.42f, 0.22f, 0.95f) : FLinearColor(0.08f, 0.08f, 0.08f, 0.95f));
    }

    if (LabelText)
    {
        LabelText->SetColorAndOpacity(FSlateColor(bSelected ? FLinearColor::White : FLinearColor(0.86f, 0.86f, 0.86f, 1.0f)));
    }
}

void UActionCombatLobbyTestOptionEntryWidget::HandleClicked()
{
    if (UActionCombatLobbyTestLoadoutWidget* Owner = OwnerWidget.Get())
    {
        Owner->HandleOptionChosen(Option);
    }
}

void UActionCombatLobbyTestLoadoutWidget::ConfigureLobbyUi(const TArray<FActionCombatLobbyCosmeticSlot>& InSlots, const TArray<FActionCombatLobbyCosmeticOption>& InOptions, const FSoftObjectPath& InTargetExperiencePath, int32 InLocalPlayerIndex, bool bInClearSlotsOnOpen)
{
    CosmeticSlots = InSlots;
    CosmeticOptions = InOptions;
    TargetExperiencePath = InTargetExperiencePath;
    LocalPlayerIndex = InLocalPlayerIndex;
    bClearSlotsOnOpen = bInClearSlotsOnOpen;
    EnsureSlotsFromOptions();

    if (!SelectedSlotTag.IsValid() && CosmeticSlots.Num() > 0)
    {
        SelectedSlotTag = CosmeticSlots[0].SlotTag;
    }

    if (bUiBuilt)
    {
        RebuildSlots();
        RebuildOptions();
        RefreshSelectionState();
    }
}

void UActionCombatLobbyTestLoadoutWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (!WidgetTree)
    {
        WidgetTree = NewObject<UWidgetTree>(this, TEXT("LobbyTestWidgetTree"));
    }

    BuildUiIfNeeded();
    SubmitDefaultNoneIfNeeded();
    RefreshSelectionState();
}

void UActionCombatLobbyTestLoadoutWidget::HandleSlotChosen(const FActionCombatLobbyCosmeticSlot& ChosenSlot)
{
    if (!ChosenSlot.SlotTag.IsValid())
    {
        return;
    }

    SelectedSlotTag = ChosenSlot.SlotTag;
    RebuildSlots();
    RebuildOptions();
    RefreshSelectionState();
}

void UActionCombatLobbyTestLoadoutWidget::HandleOptionChosen(const FActionCombatLobbyCosmeticOption& Option)
{
    FLyraLobbyPlayerLoadout Loadout;
    if (!ULyraLobbyLoadoutFunctionLibrary::GetLocalLobbyLoadout(this, Loadout, LocalPlayerIndex))
    {
        Loadout = bHasLastKnownLoadout ? LastKnownLoadout : FLyraLobbyPlayerLoadout();
    }

    if (Option.bClearSlot || Option.AccessoryData.IsNull())
    {
        Loadout = ULyraLobbyLoadoutFunctionLibrary::ClearAccessorySlot(Loadout, Option.SlotTag);
    }
    else if (UActionCombatAccessoryData* AccessoryData = Option.AccessoryData.LoadSynchronous())
    {
        Loadout = UActionCombatLobbyCosmeticLibrary::SetAccessoryDataSlot(Loadout, Option.SlotTag, AccessoryData);
    }
    else
    {
        SetStatusMessage(FText::FromString(TEXT("Accessory asset could not be loaded.")));
        return;
    }

    if (ULyraLobbyLoadoutFunctionLibrary::SubmitLocalLobbyLoadout(this, Loadout, LocalPlayerIndex))
    {
        LastKnownLoadout = Loadout;
        bHasLastKnownLoadout = true;
        SetStatusMessage(FText::Format(FText::FromString(TEXT("{0} saved.")), Option.Label));
        RefreshSelectionState();
    }
    else
    {
        SetStatusMessage(FText::FromString(TEXT("Failed to submit lobby loadout.")));
    }
}

void UActionCombatLobbyTestLoadoutWidget::BuildUiIfNeeded()
{
    if (bUiBuilt)
    {
        return;
    }

    if (!WidgetTree)
    {
        WidgetTree = NewObject<UWidgetTree>(this, TEXT("LobbyTestWidgetTree_BuildFallback"));
    }

    if (!WidgetTree)
    {
        UE_LOG(LogActionCombatLobbyTestUi, Error, TEXT("Lobby test widget tree is null, cannot build UI."));
        return;
    }

    UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
    WidgetTree->RootWidget = RootCanvas;

    UBorder* PanelBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PanelBorder"));
    PanelBorder->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.02f, 0.88f));

    UCanvasPanelSlot* BorderSlot = RootCanvas->AddChildToCanvas(PanelBorder);
    BorderSlot->SetAutoSize(true);
    BorderSlot->SetPosition(FVector2D(36.0f, 36.0f));

    USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PanelSizeBox"));
    SizeBox->SetWidthOverride(620.0f);
    PanelBorder->SetContent(SizeBox);

    RootVerticalBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RootVerticalBox"));
    SizeBox->SetContent(RootVerticalBox);

    UTextBlock* TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
    TitleText->SetText(FText::FromString(TEXT("Lobby Cosmetic Test")));
    TitleText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    if (UVerticalBoxSlot* TitleSlot = RootVerticalBox->AddChildToVerticalBox(TitleText))
    {
        TitleSlot->SetPadding(FMargin(16.0f, 16.0f, 16.0f, 4.0f));
    }

    UTextBlock* SubtitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SubtitleText"));
    SubtitleText->SetText(FText::FromString(TEXT("Choose a slot, then choose an item.")));
    SubtitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.72f, 0.72f, 0.72f, 1.0f)));
    if (UVerticalBoxSlot* SubtitleSlot = RootVerticalBox->AddChildToVerticalBox(SubtitleText))
    {
        SubtitleSlot->SetPadding(FMargin(16.0f, 0.0f, 16.0f, 8.0f));
    }

    SlotsHorizontalBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("SlotsHorizontalBox"));
    if (UVerticalBoxSlot* SlotsSlot = RootVerticalBox->AddChildToVerticalBox(SlotsHorizontalBox))
    {
        SlotsSlot->SetPadding(FMargin(12.0f, 4.0f, 12.0f, 8.0f));
    }

    SelectedSlotText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SelectedSlotText"));
    SelectedSlotText->SetText(GetSelectedSlotTitle());
    SelectedSlotText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    if (UVerticalBoxSlot* SelectedSlot = RootVerticalBox->AddChildToVerticalBox(SelectedSlotText))
    {
        SelectedSlot->SetPadding(FMargin(16.0f, 2.0f, 16.0f, 6.0f));
    }

    UScrollBox* ScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("OptionsScrollBox"));
    if (UVerticalBoxSlot* ScrollSlot = RootVerticalBox->AddChildToVerticalBox(ScrollBox))
    {
        ScrollSlot->SetPadding(FMargin(12.0f, 0.0f, 12.0f, 6.0f));
        ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
    }

    OptionsVerticalBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("OptionsVerticalBox"));
    ScrollBox->AddChild(OptionsVerticalBox);

    PlayButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("PlayButton"));
    PlayButton->SetBackgroundColor(FLinearColor(0.18f, 0.18f, 0.42f, 0.95f));
    PlayButton->OnClicked.AddDynamic(this, &ThisClass::HandlePlayClicked);
    PlayButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PlayButtonText"));
    PlayButtonText->SetText(FText::FromString(TEXT("Play")));
    PlayButtonText->SetJustification(ETextJustify::Center);
    PlayButtonText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    PlayButton->AddChild(PlayButtonText);
    if (UVerticalBoxSlot* PlaySlot = RootVerticalBox->AddChildToVerticalBox(PlayButton))
    {
        PlaySlot->SetPadding(FMargin(12.0f, 6.0f, 12.0f, 4.0f));
    }

    StatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StatusText"));
    StatusText->SetColorAndOpacity(FSlateColor(FLinearColor(0.78f, 0.78f, 0.78f, 1.0f)));
    StatusText->SetText(FText::FromString(TEXT("Waiting for lobby loadout...")));
    if (UVerticalBoxSlot* StatusSlot = RootVerticalBox->AddChildToVerticalBox(StatusText))
    {
        StatusSlot->SetPadding(FMargin(16.0f, 2.0f, 16.0f, 16.0f));
    }

    RebuildSlots();
    RebuildOptions();
    bUiBuilt = true;
    UE_LOG(LogActionCombatLobbyTestUi, Log, TEXT("Lobby cosmetic test UI built successfully."));
}

void UActionCombatLobbyTestLoadoutWidget::RebuildSlots()
{
    if (!WidgetTree || !SlotsHorizontalBox)
    {
        return;
    }

    SlotsHorizontalBox->ClearChildren();
    SlotEntries.Reset();

    for (const FActionCombatLobbyCosmeticSlot& CosmeticSlot : CosmeticSlots)
    {
        if (!CosmeticSlot.SlotTag.IsValid())
        {
            continue;
        }

        UActionCombatLobbyTestSlotEntryWidget* Entry = WidgetTree->ConstructWidget<UActionCombatLobbyTestSlotEntryWidget>(UActionCombatLobbyTestSlotEntryWidget::StaticClass());
        Entry->InitializeEntry(this, CosmeticSlot);
        SlotEntries.Add(Entry);

        if (UHorizontalBoxSlot* EntrySlot = SlotsHorizontalBox->AddChildToHorizontalBox(Entry))
        {
            EntrySlot->SetPadding(FMargin(4.0f, 2.0f, 4.0f, 2.0f));
        }
    }
}

void UActionCombatLobbyTestLoadoutWidget::RebuildOptions()
{
    if (!WidgetTree || !OptionsVerticalBox)
    {
        return;
    }

    OptionsVerticalBox->ClearChildren();
    OptionEntries.Reset();

    if (SelectedSlotText)
    {
        SelectedSlotText->SetText(GetSelectedSlotTitle());
    }

    FActionCombatLobbyCosmeticOption NoneOption;
    NoneOption.Label = FText::FromString(TEXT("None"));
    NoneOption.SlotTag = SelectedSlotTag;
    NoneOption.bClearSlot = true;

    bool bHasExplicitClearOption = false;
    for (const FActionCombatLobbyCosmeticOption& Option : CosmeticOptions)
    {
        if (Option.SlotTag == SelectedSlotTag && Option.bClearSlot)
        {
            bHasExplicitClearOption = true;
            NoneOption = Option;
            break;
        }
    }

    UActionCombatLobbyTestOptionEntryWidget* NoneEntry = WidgetTree->ConstructWidget<UActionCombatLobbyTestOptionEntryWidget>(UActionCombatLobbyTestOptionEntryWidget::StaticClass());
    NoneEntry->InitializeEntry(this, NoneOption);
    OptionEntries.Add(NoneEntry);
    if (UVerticalBoxSlot* NoneEntrySlot = OptionsVerticalBox->AddChildToVerticalBox(NoneEntry))
    {
        NoneEntrySlot->SetPadding(FMargin(4.0f, 2.0f, 4.0f, 2.0f));
    }

    for (const FActionCombatLobbyCosmeticOption& Option : CosmeticOptions)
    {
        if (Option.SlotTag != SelectedSlotTag || Option.bClearSlot || Option.AccessoryData.IsNull())
        {
            continue;
        }

        UActionCombatLobbyTestOptionEntryWidget* Entry = WidgetTree->ConstructWidget<UActionCombatLobbyTestOptionEntryWidget>(UActionCombatLobbyTestOptionEntryWidget::StaticClass());
        Entry->InitializeEntry(this, Option);
        OptionEntries.Add(Entry);

        if (UVerticalBoxSlot* EntrySlot = OptionsVerticalBox->AddChildToVerticalBox(Entry))
        {
            EntrySlot->SetPadding(FMargin(4.0f, 2.0f, 4.0f, 2.0f));
        }
    }

    if (!bHasExplicitClearOption)
    {
        UE_LOG(LogActionCombatLobbyTestUi, Verbose, TEXT("Using implicit None option for slot %s."), *SelectedSlotTag.ToString());
    }
}

void UActionCombatLobbyTestLoadoutWidget::RefreshSelectionState()
{
    FLyraLobbyPlayerLoadout Loadout;
    if (ULyraLobbyLoadoutFunctionLibrary::GetLocalLobbyLoadout(this, Loadout, LocalPlayerIndex))
    {
        LastKnownLoadout = Loadout;
        bHasLastKnownLoadout = true;
        RefreshRetryCount = 0;
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().ClearTimer(RetryRefreshTimerHandle);
        }
    }
    else if (bHasLastKnownLoadout)
    {
        Loadout = LastKnownLoadout;
    }
    else
    {
        SetStatusMessage(FText::FromString(TEXT("Waiting for lobby player state...")));

        if (RefreshRetryCount < ActionCombatLobbyUiDefaults::MaxRefreshRetries)
        {
            ++RefreshRetryCount;
            if (UWorld* World = GetWorld())
            {
                World->GetTimerManager().SetTimer(RetryRefreshTimerHandle, this, &ThisClass::RetryRefreshState, ActionCombatLobbyUiDefaults::RefreshRetryDelaySeconds, false);
            }
        }

        Loadout = FLyraLobbyPlayerLoadout();
    }

    for (UActionCombatLobbyTestSlotEntryWidget* Entry : SlotEntries)
    {
        if (Entry)
        {
            Entry->RefreshVisual(Entry->GetSlot().SlotTag == SelectedSlotTag);
        }
    }

    for (UActionCombatLobbyTestOptionEntryWidget* Entry : OptionEntries)
    {
        if (!Entry)
        {
            continue;
        }

        const FActionCombatLobbyCosmeticOption& Option = Entry->GetOption();
        bool bSelected = false;
        if (Option.bClearSlot || Option.AccessoryData.IsNull())
        {
            bSelected = !UActionCombatLobbyCosmeticLibrary::GetAccessoryIdForSlot(Loadout, Option.SlotTag).IsValid();
        }
        else if (UActionCombatAccessoryData* AccessoryData = Option.AccessoryData.LoadSynchronous())
        {
            bSelected = UActionCombatLobbyCosmeticLibrary::IsAccessoryDataSelected(Loadout, Option.SlotTag, AccessoryData);
        }

        Entry->RefreshVisual(bSelected);
    }
}

void UActionCombatLobbyTestLoadoutWidget::SubmitDefaultNoneIfNeeded()
{
    if (!bClearSlotsOnOpen || bDefaultNoneSubmitted)
    {
        return;
    }

    FLyraLobbyPlayerLoadout Loadout;
    if (!ULyraLobbyLoadoutFunctionLibrary::GetLocalLobbyLoadout(this, Loadout, LocalPlayerIndex))
    {
        Loadout = bHasLastKnownLoadout ? LastKnownLoadout : FLyraLobbyPlayerLoadout();
    }

    for (const FActionCombatLobbyCosmeticSlot& CosmeticSlot : CosmeticSlots)
    {
        if (CosmeticSlot.SlotTag.IsValid())
        {
            Loadout = ULyraLobbyLoadoutFunctionLibrary::ClearAccessorySlot(Loadout, CosmeticSlot.SlotTag);
        }
    }

    if (ULyraLobbyLoadoutFunctionLibrary::SubmitLocalLobbyLoadout(this, Loadout, LocalPlayerIndex))
    {
        LastKnownLoadout = Loadout;
        bHasLastKnownLoadout = true;
        bDefaultNoneSubmitted = true;
        SetStatusMessage(FText::FromString(TEXT("Default cosmetics cleared.")));
    }
}

void UActionCombatLobbyTestLoadoutWidget::SetStatusMessage(const FText& Message)
{
    if (StatusText)
    {
        StatusText->SetText(Message);
    }
}

FText UActionCombatLobbyTestLoadoutWidget::GetSlotTitle(const FGameplayTag& SlotTag) const
{
    const FString SlotString = SlotTag.IsValid() ? SlotTag.ToString() : FString(TEXT("Unknown"));
    int32 LastDotIndex = INDEX_NONE;
    if (SlotString.FindLastChar(TEXT('.'), LastDotIndex))
    {
        return FText::FromString(SlotString.RightChop(LastDotIndex + 1));
    }

    return FText::FromString(SlotString);
}

FText UActionCombatLobbyTestLoadoutWidget::GetSelectedSlotTitle() const
{
    for (const FActionCombatLobbyCosmeticSlot& CosmeticSlot : CosmeticSlots)
    {
        if (CosmeticSlot.SlotTag == SelectedSlotTag && !CosmeticSlot.Label.IsEmpty())
        {
            return CosmeticSlot.Label;
        }
    }

    return GetSlotTitle(SelectedSlotTag);
}

void UActionCombatLobbyTestLoadoutWidget::EnsureSlotsFromOptions()
{
    if (CosmeticSlots.Num() > 0)
    {
        return;
    }

    for (const FActionCombatLobbyCosmeticOption& Option : CosmeticOptions)
    {
        if (!Option.SlotTag.IsValid())
        {
            continue;
        }

        const bool bAlreadyAdded = CosmeticSlots.ContainsByPredicate([&Option](const FActionCombatLobbyCosmeticSlot& ExistingSlot)
        {
            return ExistingSlot.SlotTag == Option.SlotTag;
        });

        if (!bAlreadyAdded)
        {
            FActionCombatLobbyCosmeticSlot& NewSlot = CosmeticSlots.AddDefaulted_GetRef();
            NewSlot.SlotTag = Option.SlotTag;
            NewSlot.Label = GetSlotTitle(Option.SlotTag);
        }
    }
}

void UActionCombatLobbyTestLoadoutWidget::HandlePlayClicked()
{
    UObject* ExperienceObject = TargetExperiencePath.TryLoad();
    const ULyraUserFacingExperienceDefinition* Experience = (const ULyraUserFacingExperienceDefinition*)ExperienceObject;
    if (!Experience)
    {
        SetStatusMessage(FText::FromString(TEXT("Target experience could not be loaded.")));
        return;
    }

    if (ULyraLobbyLoadoutFunctionLibrary::QuickPlayLocalLobbyExperience(this, Experience, LocalPlayerIndex, ECommonSessionOnlineMode::LAN))
    {
        SetStatusMessage(FText::FromString(TEXT("Starting travel...")));
    }
    else
    {
        SetStatusMessage(FText::FromString(TEXT("Failed to start lobby travel.")));
    }
}

void UActionCombatLobbyTestLoadoutWidget::RetryRefreshState()
{
    RefreshSelectionState();
}
