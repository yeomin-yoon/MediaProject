#include "LockOnWorldSubsystem.h"

#include "LockOnSettings.h"
#include "LockOnTargetComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LockOnWorldSubsystem)

bool ULockOnWorldSubsystem::DoesSupportWorldType(EWorldType::Type WorldType) const
{
    return (WorldType == EWorldType::Game) || (WorldType == EWorldType::PIE) || (WorldType == EWorldType::GamePreview);
}

void ULockOnWorldSubsystem::RegisterTarget(ULockOnTargetComponent* TargetComponent, const FIntPoint& CellCoord)
{
    if (!IsValid(TargetComponent))
    {
        return;
    }

    RegisteredTargets.AddUnique(TargetComponent);
    AddTargetToCell(TargetComponent, CellCoord);
}

void ULockOnWorldSubsystem::UnregisterTarget(ULockOnTargetComponent* TargetComponent, const FIntPoint& CellCoord)
{
    if (!IsValid(TargetComponent))
    {
        return;
    }

    RemoveTargetFromCell(TargetComponent, CellCoord);
    RegisteredTargets.Remove(TargetComponent);
}

void ULockOnWorldSubsystem::MoveTarget(ULockOnTargetComponent* TargetComponent, const FIntPoint& OldCellCoord, const FIntPoint& NewCellCoord)
{
    if (!IsValid(TargetComponent) || (OldCellCoord == NewCellCoord))
    {
        return;
    }

    RemoveTargetFromCell(TargetComponent, OldCellCoord);
    AddTargetToCell(TargetComponent, NewCellCoord);
    ++CellReassignmentCount;
}

FIntPoint ULockOnWorldSubsystem::ComputeCellCoord(const FVector& WorldLocation) const
{
    const float CellSize = FMath::Max(1.0f, ULockOnSettings::Get()->GridCellSize);
    return FIntPoint(FMath::FloorToInt(WorldLocation.X / CellSize), FMath::FloorToInt(WorldLocation.Y / CellSize));
}

void ULockOnWorldSubsystem::QueryTargets(const FVector& Origin, float Radius, bool bUseSpatialQuery, TArray<ULockOnTargetComponent*>& OutTargets, int32& OutTotalRegistered)
{
    OutTargets.Reset();
    CompactRegisteredTargets();
    OutTotalRegistered = RegisteredTargets.Num();

    TSet<ULockOnTargetComponent*> SeenTargets;

    if (!bUseSpatialQuery)
    {
        for (const TWeakObjectPtr<ULockOnTargetComponent>& Entry : RegisteredTargets)
        {
            if (ULockOnTargetComponent* TargetComponent = Entry.Get())
            {
                SeenTargets.Add(TargetComponent);
                OutTargets.Add(TargetComponent);
            }
        }
        return;
    }

    const float CellSize = FMath::Max(1.0f, ULockOnSettings::Get()->GridCellSize);
    const int32 RadiusInCells = FMath::CeilToInt(Radius / CellSize);
    const FIntPoint OriginCell = ComputeCellCoord(Origin);

    for (int32 Y = OriginCell.Y - RadiusInCells; Y <= OriginCell.Y + RadiusInCells; ++Y)
    {
        for (int32 X = OriginCell.X - RadiusInCells; X <= OriginCell.X + RadiusInCells; ++X)
        {
            if (const TArray<TWeakObjectPtr<ULockOnTargetComponent>>* CellTargets = GridCells.Find(FIntPoint(X, Y)))
            {
                AppendValidTargets(*CellTargets, OutTargets, SeenTargets);
            }
        }
    }
}

void ULockOnWorldSubsystem::GatherAllTargets(TArray<ULockOnTargetComponent*>& OutTargets)
{
    int32 TotalRegistered = 0;
    QueryTargets(FVector::ZeroVector, 0.0f, false, OutTargets, TotalRegistered);
}

int32 ULockOnWorldSubsystem::GetRegisteredTargetCount() const
{
    return RegisteredTargets.Num();
}

void ULockOnWorldSubsystem::AddTargetToCell(ULockOnTargetComponent* TargetComponent, const FIntPoint& CellCoord)
{
    GridCells.FindOrAdd(CellCoord).AddUnique(TargetComponent);
}

void ULockOnWorldSubsystem::RemoveTargetFromCell(ULockOnTargetComponent* TargetComponent, const FIntPoint& CellCoord)
{
    if (TArray<TWeakObjectPtr<ULockOnTargetComponent>>* CellTargets = GridCells.Find(CellCoord))
    {
        CellTargets->Remove(TargetComponent);
        if (CellTargets->Num() == 0)
        {
            GridCells.Remove(CellCoord);
        }
    }
}

void ULockOnWorldSubsystem::AppendValidTargets(const TArray<TWeakObjectPtr<ULockOnTargetComponent>>& InTargets, TArray<ULockOnTargetComponent*>& OutTargets, TSet<ULockOnTargetComponent*>& InOutSeenTargets)
{
    for (const TWeakObjectPtr<ULockOnTargetComponent>& Entry : InTargets)
    {
        if (ULockOnTargetComponent* TargetComponent = Entry.Get())
        {
            if (!InOutSeenTargets.Contains(TargetComponent))
            {
                InOutSeenTargets.Add(TargetComponent);
                OutTargets.Add(TargetComponent);
            }
        }
    }
}

void ULockOnWorldSubsystem::CompactRegisteredTargets()
{
    RegisteredTargets.RemoveAll([](const TWeakObjectPtr<ULockOnTargetComponent>& Entry)
    {
        return !Entry.IsValid();
    });
}
