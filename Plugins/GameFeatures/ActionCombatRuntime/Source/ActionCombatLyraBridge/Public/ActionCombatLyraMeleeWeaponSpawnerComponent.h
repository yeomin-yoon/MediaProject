#pragma once

#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"

#include "ActionCombatLyraMeleeWeaponSpawnerComponent.generated.h"

class AActionCombatMeleeWeaponActor;
class USceneComponent;

UCLASS(BlueprintType, ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class ACTIONCOMBATLYRABRIDGE_API UActionCombatLyraMeleeWeaponSpawnerComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UActionCombatLyraMeleeWeaponSpawnerComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Action Combat|Weapon")
    AActionCombatMeleeWeaponActor* SpawnWeapon();

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Action Combat|Weapon")
    void DestroyWeapon();

    UFUNCTION(BlueprintPure, Category = "Action Combat|Weapon")
    AActionCombatMeleeWeaponActor* GetSpawnedWeapon() const;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Weapon")
    bool bSpawnOnBeginPlay = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Weapon")
    bool bDestroyWeaponOnEndPlay = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Weapon")
    TSubclassOf<AActionCombatMeleeWeaponActor> WeaponActorClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Weapon")
    FComponentReference AttachParentComponent;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Weapon")
    FName AttachSocket = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Weapon")
    FTransform RelativeAttachTransform;

private:
    USceneComponent* ResolveAttachParent() const;

    UPROPERTY(Transient)
    TObjectPtr<AActionCombatMeleeWeaponActor> SpawnedWeaponActor;
};
