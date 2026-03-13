#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"

#include "LockOnWorldSubsystem.generated.h"

class ULockOnTargetComponent;

UCLASS()
class LOCKONSYSTEMRUNTIME_API ULockOnWorldSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;

    void RegisterTarget(ULockOnTargetComponent* TargetComponent, const FIntPoint& CellCoord);
    void UnregisterTarget(ULockOnTargetComponent* TargetComponent, const FIntPoint& CellCoord);
    void MoveTarget(ULockOnTargetComponent* TargetComponent, const FIntPoint& OldCellCoord, const FIntPoint& NewCellCoord);

    FIntPoint ComputeCellCoord(const FVector& WorldLocation) const;

    void QueryTargets(const FVector& Origin, float Radius, bool bUseSpatialQuery, TArray<ULockOnTargetComponent*>& OutTargets, int32& OutTotalRegistered);
    void GatherAllTargets(TArray<ULockOnTargetComponent*>& OutTargets);

    int32 GetRegisteredTargetCount() const;
    int32 GetCellReassignmentCount() const { return CellReassignmentCount; }

private:
    void AddTargetToCell(ULockOnTargetComponent* TargetComponent, const FIntPoint& CellCoord);
    void RemoveTargetFromCell(ULockOnTargetComponent* TargetComponent, const FIntPoint& CellCoord);
    void AppendValidTargets(const TArray<TWeakObjectPtr<ULockOnTargetComponent>>& InTargets, TArray<ULockOnTargetComponent*>& OutTargets, TSet<ULockOnTargetComponent*>& InOutSeenTargets);
    void CompactRegisteredTargets();

private:
    TMap<FIntPoint, TArray<TWeakObjectPtr<ULockOnTargetComponent>>> GridCells;
    TArray<TWeakObjectPtr<ULockOnTargetComponent>> RegisteredTargets;
    int32 CellReassignmentCount = 0;
};
