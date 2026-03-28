#pragma once

#include "GameFramework/Actor.h"

#include "ActionCombatMeleeWeaponActor.generated.h"

class UActionCombatMeleeTraceComponent;
class USkeletalMeshComponent;
class USceneComponent;

UCLASS(Blueprintable)
class ACTIONCOMBATRUNTIME_API AActionCombatMeleeWeaponActor : public AActor
{
    GENERATED_BODY()

public:
    AActionCombatMeleeWeaponActor(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    UFUNCTION(BlueprintPure, Category = "Action Combat|Weapon")
    USkeletalMeshComponent* GetWeaponMesh() const
    {
        return WeaponMesh;
    }

    UFUNCTION(BlueprintPure, Category = "Action Combat|Weapon")
    UActionCombatMeleeTraceComponent* GetMeleeTraceComponent() const
    {
        return MeleeTraceComponent;
    }

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USkeletalMeshComponent> WeaponMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UActionCombatMeleeTraceComponent> MeleeTraceComponent;
};
