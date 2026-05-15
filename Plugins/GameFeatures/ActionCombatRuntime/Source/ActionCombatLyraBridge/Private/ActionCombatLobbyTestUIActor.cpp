#include "ActionCombatLobbyTestUIActor.h"

#include "ActionCombatAccessoryData.h"

#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

AActionCombatLobbyTestUIActor::AActionCombatLobbyTestUIActor(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = false;

    WidgetClass = UActionCombatLobbyTestLoadoutWidget::StaticClass();
    TargetExperiencePath = FSoftObjectPath(TEXT("/Game/1dev/OS/UI/DA_LobbyPlay_ActionCombatTest.DA_LobbyPlay_ActionCombatTest"));

    FActionCombatLobbyCosmeticSlot HeadSlot;
    HeadSlot.Label = FText::FromString(TEXT("Head"));
    HeadSlot.SlotTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Cosmetic.Slot.Head")), false);
    CosmeticSlots.Add(HeadSlot);

    FActionCombatLobbyCosmeticSlot FaceSlot;
    FaceSlot.Label = FText::FromString(TEXT("Face"));
    FaceSlot.SlotTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Cosmetic.Slot.Face")), false);
    CosmeticSlots.Add(FaceSlot);

    FActionCombatLobbyCosmeticSlot BackSlot;
    BackSlot.Label = FText::FromString(TEXT("Back"));
    BackSlot.SlotTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Cosmetic.Slot.Back")), false);
    CosmeticSlots.Add(BackSlot);

    FActionCombatLobbyCosmeticOption NoneOption;
    NoneOption.Label = FText::FromString(TEXT("None"));
    NoneOption.SlotTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Cosmetic.Slot.Head")), false);
    NoneOption.bClearSlot = true;
    CosmeticOptions.Add(NoneOption);

    FActionCombatLobbyCosmeticOption HeadCubeOption;
    HeadCubeOption.Label = FText::FromString(TEXT("Head Cube"));
    HeadCubeOption.SlotTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Cosmetic.Slot.Head")), false);
    HeadCubeOption.AccessoryData = TSoftObjectPtr<UActionCombatAccessoryData>(FSoftObjectPath(TEXT("/Game/1dev/OS/DragonKnightRuntime/DA_DragonKnightRuntime_TestAccessory_HeadCube.DA_DragonKnightRuntime_TestAccessory_HeadCube")));
    CosmeticOptions.Add(HeadCubeOption);
}

void AActionCombatLobbyTestUIActor::BeginPlay()
{
    Super::BeginPlay();
    TryCreateLobbyUi();
}

void AActionCombatLobbyTestUIActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(CreateWidgetRetryTimerHandle);
    }

    if (ActiveWidget)
    {
        ActiveWidget->RemoveFromParent();
        ActiveWidget = nullptr;
    }

    Super::EndPlay(EndPlayReason);
}

void AActionCombatLobbyTestUIActor::TryCreateLobbyUi()
{
    if (ActiveWidget)
    {
        return;
    }

    APlayerController* LocalPlayerController = UGameplayStatics::GetPlayerController(this, 0);
    if (!LocalPlayerController || !LocalPlayerController->IsLocalController())
    {
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().SetTimer(CreateWidgetRetryTimerHandle, this, &ThisClass::TryCreateLobbyUi, 0.25f, false);
        }
        return;
    }

    TSubclassOf<UActionCombatLobbyTestLoadoutWidget> WidgetClassToCreate = WidgetClass;
    if (!WidgetClassToCreate)
    {
        WidgetClassToCreate = UActionCombatLobbyTestLoadoutWidget::StaticClass();
    }
    UActionCombatLobbyTestLoadoutWidget* CreatedWidget = CreateWidget<UActionCombatLobbyTestLoadoutWidget>(LocalPlayerController, WidgetClassToCreate);
    if (!CreatedWidget)
    {
        return;
    }

    CreatedWidget->ConfigureLobbyUi(CosmeticSlots, CosmeticOptions, TargetExperiencePath, 0, bClearSlotsOnOpen);
    CreatedWidget->AddToPlayerScreen(ViewportZOrder);
    ActiveWidget = CreatedWidget;

    LocalPlayerController->bShowMouseCursor = true;
    FInputModeGameAndUI InputMode;
    InputMode.SetWidgetToFocus(CreatedWidget->TakeWidget());
    InputMode.SetHideCursorDuringCapture(false);
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    LocalPlayerController->SetInputMode(InputMode);
}
