#pragma once

#include "Abilities/GameplayAbilityTargetTypes.h"
#include "GameplayTagContainer.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "ActionCombatMeleeTraceTypes.h"
#include "ActionCombatBlueprintLibrary.generated.h"

class AActor;
class UActionCombatAccessoryData;
class UActionCombatAppearanceData;
class UActionCombatMeleeTraceComponent;
class UAnimInstance;
class UMaterialInterface;
class USkeletalMesh;
class UStaticMesh;

USTRUCT(BlueprintType)
struct ACTIONCOMBATRUNTIME_API FActionCombatNamedAppearanceSlotDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    FName SlotTagName = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    FName TargetComponentName = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    FName AttachSocket = NAME_None;
};

UCLASS()
class ACTIONCOMBATRUNTIME_API UActionCombatBlueprintLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category = "Action Combat|Tags")
    static FGameplayTag RequestGameplayTagByName(FName TagName, bool bErrorIfNotFound = false);

    UFUNCTION(BlueprintPure, Category = "Action Combat|Tags")
    static FGameplayTagContainer MakeGameplayTagContainerFromNames(const TArray<FName>& TagNames, bool bErrorIfNotFound = false);

    UFUNCTION(BlueprintCallable, Category = "Action Combat|Appearance")
    static void ConfigureAppearanceData(
        UActionCombatAppearanceData* AppearanceData,
        FName DefaultPrimaryVisualMeshComponentName,
        FName DefaultSourceMeshComponentName,
        const TArray<FName>& AppearanceTagNames,
        const TArray<FActionCombatNamedAppearanceSlotDefinition>& SlotDefinitions);

    UFUNCTION(BlueprintCallable, Category = "Action Combat|Accessories")
    static void ConfigureAccessoryData(
        UActionCombatAccessoryData* AccessoryData,
        FName SlotTagName,
        const TArray<FName>& RequiredAppearanceTagNames,
        const TArray<FName>& BlockedAppearanceTagNames,
        UStaticMesh* StaticMesh,
        USkeletalMesh* SkeletalMesh,
        TSubclassOf<UAnimInstance> AnimClass,
        const TArray<UMaterialInterface*>& MaterialOverrides,
        FName SocketOverride,
        const FTransform& RelativeTransform);

    UFUNCTION(BlueprintCallable, Category = "Action Combat|Melee Trace", meta = (DefaultToSelf = "Actor"))
    static UActionCombatMeleeTraceComponent* FindMeleeTraceComponent(AActor* Actor, FName TraceSourceId = NAME_None, bool bIncludeAttachedActors = true);

    UFUNCTION(BlueprintCallable, Category = "Action Combat|Melee Trace", meta = (DefaultToSelf = "Actor"))
    static TArray<UActionCombatMeleeTraceComponent*> FindMeleeTraceComponents(AActor* Actor, FName TraceSourceId = NAME_None, bool bIncludeAttachedActors = true);

    UFUNCTION(BlueprintCallable, Category = "Action Combat|Melee Trace")
    static FGameplayAbilityTargetDataHandle MakeTargetDataFromHitResult(const FHitResult& HitResult);

    UFUNCTION(BlueprintCallable, Category = "Action Combat|Melee Trace")
    static FGameplayAbilityTargetDataHandle MakeTargetDataFromHitResults(const TArray<FHitResult>& HitResults);

    UFUNCTION(BlueprintCallable, Category = "Action Combat|Melee Trace")
    static FGameplayAbilityTargetDataHandle MakeTargetDataFromRecordedHit(const FActionCombatRecordedHit& RecordedHit);

    UFUNCTION(BlueprintCallable, Category = "Action Combat|Melee Trace")
    static FGameplayAbilityTargetDataHandle MakeTargetDataFromRecordedHits(const TArray<FActionCombatRecordedHit>& RecordedHits);
};
