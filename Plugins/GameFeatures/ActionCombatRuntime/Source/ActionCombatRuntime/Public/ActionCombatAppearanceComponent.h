#pragma once

#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "GameplayTagContainer.h"

#include "ActionCombatAppearanceComponent.generated.h"

class UActionCombatAppearanceData;
class USceneComponent;
class USkeletalMeshComponent;

UCLASS(BlueprintType, ClassGroup = (Cosmetics), meta = (BlueprintSpawnableComponent))
class ACTIONCOMBATRUNTIME_API UActionCombatAppearanceComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UActionCombatAppearanceComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    virtual void BeginPlay() override;

    UFUNCTION(BlueprintPure, Category = "Action Combat|Appearance")
    UActionCombatAppearanceData* GetAppearanceData() const
    {
        return AppearanceData;
    }

    UFUNCTION(BlueprintCallable, Category = "Action Combat|Appearance")
    void SetAppearanceData(UActionCombatAppearanceData* NewAppearanceData);

    UFUNCTION(BlueprintPure, Category = "Action Combat|Appearance")
    USkeletalMeshComponent* GetPrimaryVisualMesh() const;

    UFUNCTION(BlueprintPure, Category = "Action Combat|Appearance")
    USkeletalMeshComponent* GetSourceMesh() const;

    UFUNCTION(BlueprintPure, Category = "Action Combat|Appearance")
    FGameplayTagContainer GetAppearanceTags() const;

    UFUNCTION(BlueprintCallable, Category = "Action Combat|Appearance")
    void ApplySourceMeshVisibilityRules();

    bool TryResolveAccessoryAttachment(const FGameplayTag& SlotTag, USceneComponent*& OutAttachComponent, FName& OutAttachSocket) const;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
    TObjectPtr<UActionCombatAppearanceData> AppearanceData = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
    FComponentReference PrimaryVisualMeshComponent;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
    FComponentReference SourceMeshComponent;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
    bool bAutoApplySourceMeshVisibilityRules = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
    bool bHideSourceMeshInGame = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
    bool bDisableSourceMeshShadow = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
    bool bForceSourceMeshAlwaysTickPose = true;

private:
    USkeletalMeshComponent* ResolveMeshFromReference(const FComponentReference& ComponentReference) const;
    USceneComponent* ResolveSceneComponentByName(FName ComponentName) const;
    USkeletalMeshComponent* ResolveSourceMeshByFallback() const;
    USkeletalMeshComponent* ResolvePrimaryVisualMeshByFallback() const;
};
