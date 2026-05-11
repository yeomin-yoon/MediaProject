#include "ActionCombatLobbyCosmeticLibrary.h"

#include "ActionCombatAccessoryData.h"

FPrimaryAssetId UActionCombatLobbyCosmeticLibrary::MakeAccessoryPrimaryAssetId(const UActionCombatAccessoryData* AccessoryData)
{
    return AccessoryData ? AccessoryData->GetPrimaryAssetId() : FPrimaryAssetId();
}

FLyraLobbyPlayerLoadout UActionCombatLobbyCosmeticLibrary::SetAccessoryDataSlot(FLyraLobbyPlayerLoadout Loadout, FGameplayTag SlotTag, const UActionCombatAccessoryData* AccessoryData)
{
    if (!SlotTag.IsValid())
    {
        return Loadout;
    }

    const FPrimaryAssetId AccessoryId = MakeAccessoryPrimaryAssetId(AccessoryData);
    if (!AccessoryId.IsValid())
    {
        Loadout.AccessorySlots.RemoveAll([SlotTag](const FLyraLobbyAccessorySelection& Selection)
        {
            return Selection.SlotTag == SlotTag;
        });
        return Loadout;
    }

    for (FLyraLobbyAccessorySelection& Selection : Loadout.AccessorySlots)
    {
        if (Selection.SlotTag == SlotTag)
        {
            Selection.AccessoryId = AccessoryId;
            return Loadout;
        }
    }

    FLyraLobbyAccessorySelection& NewSelection = Loadout.AccessorySlots.AddDefaulted_GetRef();
    NewSelection.SlotTag = SlotTag;
    NewSelection.AccessoryId = AccessoryId;
    return Loadout;
}

FPrimaryAssetId UActionCombatLobbyCosmeticLibrary::GetAccessoryIdForSlot(const FLyraLobbyPlayerLoadout& Loadout, FGameplayTag SlotTag)
{
    if (!SlotTag.IsValid())
    {
        return FPrimaryAssetId();
    }

    for (const FLyraLobbyAccessorySelection& Selection : Loadout.AccessorySlots)
    {
        if (Selection.SlotTag == SlotTag)
        {
            return Selection.AccessoryId;
        }
    }

    return FPrimaryAssetId();
}

bool UActionCombatLobbyCosmeticLibrary::IsAccessoryDataSelected(const FLyraLobbyPlayerLoadout& Loadout, FGameplayTag SlotTag, const UActionCombatAccessoryData* AccessoryData)
{
    if (!AccessoryData)
    {
        return !GetAccessoryIdForSlot(Loadout, SlotTag).IsValid();
    }

    return GetAccessoryIdForSlot(Loadout, SlotTag) == AccessoryData->GetPrimaryAssetId();
}
