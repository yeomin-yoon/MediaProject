#include "ActionCombatLobbyCosmeticBridgeComponent.h"

#include "ActionCombatAccessoryComponent.h"
#include "ActionCombatAccessoryData.h"
#include "ActionCombatRuntimeLog.h"

#include "Engine/AssetManager.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Lobby/LyraLobbyPlayerStateComponent.h"
#include "Lobby/LyraLobbyTypes.h"
#include "TimerManager.h"
#include "UObject/SoftObjectPath.h"

UActionCombatLobbyCosmeticBridgeComponent::UActionCombatLobbyCosmeticBridgeComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UActionCombatLobbyCosmeticBridgeComponent::BeginPlay()
{
    Super::BeginPlay();

    if (bApplyOnBeginPlay)
    {
        ScheduleRetryIfNeeded();
    }
}

void UActionCombatLobbyCosmeticBridgeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    UnbindLobbyLoadoutChanged();

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(RetryTimerHandle);
    }

    Super::EndPlay(EndPlayReason);
}

void UActionCombatLobbyCosmeticBridgeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

bool UActionCombatLobbyCosmeticBridgeComponent::ApplyLobbyCosmeticsNow()
{
    ++ApplyAttempts;

    AActor* Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority())
    {
        return false;
    }

    ULyraLobbyPlayerStateComponent* LobbyPlayer = ResolveLobbyPlayerStateComponent();
    if (!LobbyPlayer)
    {
        LogFlow(TEXT("Waiting for LobbyPlayerStateComponent."));
        ScheduleRetryIfNeeded();
        return false;
    }

    BindLobbyLoadoutChanged(LobbyPlayer);

    UActionCombatAccessoryComponent* AccessoryComponent = ResolveAccessoryComponent();
    if (!AccessoryComponent)
    {
        LogFlow(TEXT("Waiting for ActionCombatAccessoryComponent."));
        ScheduleRetryIfNeeded();
        return false;
    }

    const FLyraLobbyPlayerLoadout& Loadout = LobbyPlayer->GetLobbyLoadout();
    const bool bForceInitialReapply = SuccessfulInitialReapplyAttempts < InitialForcedReapplyAttempts;
    if (LastAppliedRevision == Loadout.Revision && !bForceInitialReapply)
    {
        return true;
    }

    const bool bApplied = ApplyLoadout(Loadout);
    if (bApplied)
    {
        LastAppliedRevision = Loadout.Revision;
        ++SuccessfulInitialReapplyAttempts;

        if (bForceInitialReapply)
        {
            ScheduleRetryIfNeeded();
        }
    }

    return bApplied;
}

UActionCombatAccessoryComponent* UActionCombatLobbyCosmeticBridgeComponent::ResolveAccessoryComponent() const
{
    if (AActor* Owner = GetOwner())
    {
        if (UActionCombatAccessoryComponent* ExplicitComponent = Cast<UActionCombatAccessoryComponent>(AccessoryComponentReference.GetComponent(Owner)))
        {
            return ExplicitComponent;
        }

        return Owner->FindComponentByClass<UActionCombatAccessoryComponent>();
    }

    return nullptr;
}

ULyraLobbyPlayerStateComponent* UActionCombatLobbyCosmeticBridgeComponent::ResolveLobbyPlayerStateComponent() const
{
    const APawn* PawnOwner = Cast<APawn>(GetOwner());
    const APlayerState* PlayerState = PawnOwner ? PawnOwner->GetPlayerState() : nullptr;
    return PlayerState ? PlayerState->FindComponentByClass<ULyraLobbyPlayerStateComponent>() : nullptr;
}

UActionCombatAccessoryData* UActionCombatLobbyCosmeticBridgeComponent::ResolveAccessoryData(const FLyraLobbyAccessorySelection& Selection) const
{
    if (!Selection.AccessoryId.IsValid())
    {
        return nullptr;
    }

    UAssetManager& AssetManager = UAssetManager::Get();
    if (UObject* ExistingObject = AssetManager.GetPrimaryAssetObject(Selection.AccessoryId))
    {
        return Cast<UActionCombatAccessoryData>(ExistingObject);
    }

    const FSoftObjectPath AssetPath = AssetManager.GetPrimaryAssetPath(Selection.AccessoryId);
    if (!AssetPath.IsValid())
    {
        UE_LOG(LogActionCombatRuntime, Warning, TEXT("Lobby cosmetic bridge could not resolve primary asset path for %s."), *Selection.AccessoryId.ToString());
        return nullptr;
    }

    UObject* LoadedObject = AssetPath.TryLoad();
    return Cast<UActionCombatAccessoryData>(LoadedObject);
}

bool UActionCombatLobbyCosmeticBridgeComponent::ApplyLoadout(const FLyraLobbyPlayerLoadout& Loadout)
{
    UActionCombatAccessoryComponent* AccessoryComponent = ResolveAccessoryComponent();
    if (!AccessoryComponent)
    {
        return false;
    }

    TMap<FGameplayTag, UActionCombatAccessoryData*> DesiredAccessories;
    for (const FLyraLobbyAccessorySelection& Selection : Loadout.AccessorySlots)
    {
        if (!Selection.SlotTag.IsValid())
        {
            continue;
        }

        UActionCombatAccessoryData* AccessoryData = ResolveAccessoryData(Selection);
        if (!AccessoryData)
        {
            UE_LOG(LogActionCombatRuntime, Warning, TEXT("Lobby cosmetic bridge skipped slot %s because accessory %s could not be loaded."),
                *Selection.SlotTag.ToString(),
                *Selection.AccessoryId.ToString());
            continue;
        }

        DesiredAccessories.Add(Selection.SlotTag, AccessoryData);
    }

    if (bClearExistingAccessoriesBeforeApply)
    {
        if (DesiredAccessories.IsEmpty())
        {
            AccessoryComponent->RequestClearAllAccessories();
        }
        else
        {
            const TArray<FActionCombatEquippedAccessoryView> CurrentAccessories = AccessoryComponent->GetEquippedAccessories();
            for (const FActionCombatEquippedAccessoryView& CurrentAccessory : CurrentAccessories)
            {
                UActionCombatAccessoryData* const* DesiredAccessory = DesiredAccessories.Find(CurrentAccessory.SlotTag);
                if (!DesiredAccessory || !*DesiredAccessory)
                {
                    AccessoryComponent->RequestUnequipSlot(CurrentAccessory.SlotTag);
                    continue;
                }

                const FSoftObjectPath CurrentPath = CurrentAccessory.AccessoryData.ToSoftObjectPath();
                const FSoftObjectPath DesiredPath(*DesiredAccessory);
                if (CurrentPath != DesiredPath)
                {
                    AccessoryComponent->RequestUnequipSlot(CurrentAccessory.SlotTag);
                }
            }
        }
    }

    int32 AppliedCount = 0;
    for (const TPair<FGameplayTag, UActionCombatAccessoryData*>& DesiredAccessory : DesiredAccessories)
    {
        if (AccessoryComponent->RequestEquipAccessory(DesiredAccessory.Value))
        {
            ++AppliedCount;
        }
    }

    LogFlow(FString::Printf(TEXT("Applied lobby cosmetics. Revision=%d Slots=%d Applied=%d"),
        Loadout.Revision,
        Loadout.AccessorySlots.Num(),
        AppliedCount));

    if (AActor* Owner = GetOwner())
    {
        Owner->ForceNetUpdate();
    }

    return true;
}

void UActionCombatLobbyCosmeticBridgeComponent::ScheduleRetryIfNeeded()
{
    if (ApplyAttempts >= MaxApplyAttempts)
    {
        LogFlow(FString::Printf(TEXT("Stopped retrying after %d attempts."), ApplyAttempts));
        return;
    }

    UWorld* World = GetWorld();
    if (!World || World->GetTimerManager().IsTimerActive(RetryTimerHandle))
    {
        return;
    }

    World->GetTimerManager().SetTimer(
        RetryTimerHandle,
        this,
        &ThisClass::RetryApplyLobbyCosmetics,
        FMath::Max(0.01f, RetryIntervalSeconds),
        false);
}

void UActionCombatLobbyCosmeticBridgeComponent::RetryApplyLobbyCosmetics()
{
    ApplyLobbyCosmeticsNow();
}

void UActionCombatLobbyCosmeticBridgeComponent::BindLobbyLoadoutChanged(ULyraLobbyPlayerStateComponent* LobbyPlayer)
{
    if (!LobbyPlayer || BoundLobbyPlayerStateComponent.Get() == LobbyPlayer)
    {
        return;
    }

    UnbindLobbyLoadoutChanged();
    LobbyPlayer->OnLobbyLoadoutChanged.AddDynamic(this, &ThisClass::HandleLobbyLoadoutChanged);
    BoundLobbyPlayerStateComponent = LobbyPlayer;
}

void UActionCombatLobbyCosmeticBridgeComponent::UnbindLobbyLoadoutChanged()
{
    if (ULyraLobbyPlayerStateComponent* BoundLobbyPlayer = BoundLobbyPlayerStateComponent.Get())
    {
        BoundLobbyPlayer->OnLobbyLoadoutChanged.RemoveDynamic(this, &ThisClass::HandleLobbyLoadoutChanged);
    }

    BoundLobbyPlayerStateComponent.Reset();
}

void UActionCombatLobbyCosmeticBridgeComponent::HandleLobbyLoadoutChanged(const FLyraLobbyPlayerLoadout& LobbyLoadout)
{
    AActor* Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority())
    {
        return;
    }

    if (LastAppliedRevision == LobbyLoadout.Revision)
    {
        return;
    }

    if (ApplyLoadout(LobbyLoadout))
    {
        LastAppliedRevision = LobbyLoadout.Revision;
    }
}

void UActionCombatLobbyCosmeticBridgeComponent::LogFlow(const FString& Message) const
{
    if (bLogFlow)
    {
        UE_LOG(LogActionCombatRuntime, Log, TEXT("[LobbyCosmeticBridge:%s] %s"), *GetPathNameSafe(GetOwner()), *Message);
    }
}
