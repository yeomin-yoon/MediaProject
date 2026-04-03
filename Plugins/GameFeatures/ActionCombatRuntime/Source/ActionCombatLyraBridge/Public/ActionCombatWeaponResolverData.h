#pragma once

#include "Engine/DataAsset.h"
#include "Weapons/LyraWeaponInstance.h"

#include "ActionCombatWeaponResolverData.generated.h"

class UActionCombatWeaponDefinition;

USTRUCT(BlueprintType)
struct ACTIONCOMBATLYRABRIDGE_API FActionCombatWeaponResolverEntry
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Resolver")
    TSubclassOf<ULyraWeaponInstance> WeaponInstanceClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Resolver")
    TObjectPtr<UActionCombatWeaponDefinition> WeaponDefinition = nullptr;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Resolver", meta = (ClampMin = "0"))
    int32 WeaponLevel = 0;

    bool Matches(const UClass* InWeaponClass) const
    {
        return (InWeaponClass != nullptr) && WeaponInstanceClass && InWeaponClass->IsChildOf(WeaponInstanceClass);
    }
};

UCLASS(BlueprintType)
class ACTIONCOMBATLYRABRIDGE_API UActionCombatWeaponResolverData : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    const FActionCombatWeaponResolverEntry* FindEntryForWeaponClass(const UClass* WeaponClass) const;

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Resolver")
    TArray<FActionCombatWeaponResolverEntry> WeaponEntries;
};
