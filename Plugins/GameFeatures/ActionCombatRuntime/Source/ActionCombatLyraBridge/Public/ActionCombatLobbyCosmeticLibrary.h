#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "Lobby/LyraLobbyTypes.h"

#include "ActionCombatLobbyCosmeticLibrary.generated.h"

class UActionCombatAccessoryData;

UCLASS()
class ACTIONCOMBATLYRABRIDGE_API UActionCombatLobbyCosmeticLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Action Combat|Lobby Cosmetics")
    static FPrimaryAssetId MakeAccessoryPrimaryAssetId(const UActionCombatAccessoryData* AccessoryData);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Action Combat|Lobby Cosmetics")
    static FLyraLobbyPlayerLoadout SetAccessoryDataSlot(FLyraLobbyPlayerLoadout Loadout, FGameplayTag SlotTag, const UActionCombatAccessoryData* AccessoryData);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Action Combat|Lobby Cosmetics")
    static FPrimaryAssetId GetAccessoryIdForSlot(const FLyraLobbyPlayerLoadout& Loadout, FGameplayTag SlotTag);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Action Combat|Lobby Cosmetics")
    static bool IsAccessoryDataSelected(const FLyraLobbyPlayerLoadout& Loadout, FGameplayTag SlotTag, const UActionCombatAccessoryData* AccessoryData);
};
