#pragma once

#include "Abilities/GameplayAbilityTargetTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "ActionCombatMeleeTraceTypes.h"
#include "ActionCombatBlueprintLibrary.generated.h"

class AActor;
class UActionCombatMeleeTraceComponent;

UCLASS()
class ACTIONCOMBATRUNTIME_API UActionCombatBlueprintLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
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
