#include "ActionCombatLyraEquipmentResolver.h"

#include "ActionCombatWeaponDefinition.h"
#include "ActionCombatWeaponResolverData.h"
#include "Equipment/LyraEquipmentManagerComponent.h"
#include "GameFramework/Actor.h"
#include "Weapons/LyraWeaponInstance.h"

ULyraEquipmentManagerComponent* UActionCombatLyraEquipmentResolver::FindEquipmentManager(const AActor* Actor)
{
    return Actor ? Actor->FindComponentByClass<ULyraEquipmentManagerComponent>() : nullptr;
}

ULyraWeaponInstance* UActionCombatLyraEquipmentResolver::ResolveEquippedWeaponInstance(const AActor* Actor)
{
    if (ULyraEquipmentManagerComponent* EquipmentManager = FindEquipmentManager(Actor))
    {
        return EquipmentManager->GetFirstInstanceOfType<ULyraWeaponInstance>();
    }

    return nullptr;
}

const FActionCombatWeaponResolverEntry* UActionCombatLyraEquipmentResolver::ResolveEquippedWeaponEntry(const AActor* Actor, const UActionCombatWeaponResolverData* WeaponResolverData)
{
    if ((WeaponResolverData == nullptr) || (Actor == nullptr))
    {
        return nullptr;
    }

    if (const ULyraWeaponInstance* WeaponInstance = ResolveEquippedWeaponInstance(Actor))
    {
        return WeaponResolverData->FindEntryForWeaponClass(WeaponInstance->GetClass());
    }

    return nullptr;
}

UActionCombatWeaponDefinition* UActionCombatLyraEquipmentResolver::ResolveEquippedWeaponDefinition(const AActor* Actor, const UActionCombatWeaponResolverData* WeaponResolverData)
{
    if (const FActionCombatWeaponResolverEntry* WeaponEntry = ResolveEquippedWeaponEntry(Actor, WeaponResolverData))
    {
        return WeaponEntry->WeaponDefinition;
    }

    return nullptr;
}

int32 UActionCombatLyraEquipmentResolver::ResolveEquippedWeaponLevel(const AActor* Actor, const UActionCombatWeaponResolverData* WeaponResolverData)
{
    if (const FActionCombatWeaponResolverEntry* WeaponEntry = ResolveEquippedWeaponEntry(Actor, WeaponResolverData))
    {
        return FMath::Max(WeaponEntry->WeaponLevel, 0);
    }

    return 0;
}
