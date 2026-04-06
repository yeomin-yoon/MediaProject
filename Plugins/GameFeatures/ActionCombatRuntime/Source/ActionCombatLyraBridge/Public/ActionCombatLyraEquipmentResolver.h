#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"

#include "ActionCombatLyraEquipmentResolver.generated.h"

class AActor;
class UActionCombatWeaponDefinition;
class UActionCombatWeaponResolverData;
class ULyraEquipmentManagerComponent;
class ULyraWeaponInstance;
struct FActionCombatWeaponResolverEntry;

UCLASS()
class ACTIONCOMBATLYRABRIDGE_API UActionCombatLyraEquipmentResolver : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category = "Action Combat|Equipment")
    static ULyraEquipmentManagerComponent* FindEquipmentManager(const AActor* Actor);

    UFUNCTION(BlueprintPure, Category = "Action Combat|Equipment")
    static ULyraWeaponInstance* ResolveEquippedWeaponInstance(const AActor* Actor);

    static const FActionCombatWeaponResolverEntry* ResolveEquippedWeaponEntry(const AActor* Actor, const UActionCombatWeaponResolverData* WeaponResolverData);

    UFUNCTION(BlueprintPure, Category = "Action Combat|Equipment")
    static UActionCombatWeaponDefinition* ResolveEquippedWeaponDefinition(const AActor* Actor, const UActionCombatWeaponResolverData* WeaponResolverData);

    UFUNCTION(BlueprintPure, Category = "Action Combat|Equipment")
    static int32 ResolveEquippedWeaponLevel(const AActor* Actor, const UActionCombatWeaponResolverData* WeaponResolverData);
};
