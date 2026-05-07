#pragma once

#include "Toolkits/AssetEditorToolkit.h"
#include "UObject/GCObject.h"

class IDetailsView;
class SDockTab;
class SActionCombatStyleGraphView;
class UActionCombatStyleData;

class FActionCombatStyleAssetEditor : public FAssetEditorToolkit, public FGCObject
{
public:
    void InitStyleAssetEditor(const EToolkitMode::Type Mode, const TSharedPtr<class IToolkitHost>& InitToolkitHost, UActionCombatStyleData* InStyleData);

    virtual FName GetToolkitFName() const override;
    virtual FText GetBaseToolkitName() const override;
    virtual FText GetToolkitName() const override;
    virtual FString GetWorldCentricTabPrefix() const override;
    virtual FLinearColor GetWorldCentricTabColorScale() const override;
    virtual void RegisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager) override;
    virtual void UnregisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager) override;

    virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
    virtual FString GetReferencerName() const override;

private:
    TSharedRef<SDockTab> SpawnGraphTab(const class FSpawnTabArgs& Args);
    TSharedRef<SDockTab> SpawnDetailsTab(const class FSpawnTabArgs& Args);
    void HandleFinishedChangingProperties(const struct FPropertyChangedEvent& PropertyChangedEvent);
    void HandleGraphEdited();

private:
    static const FName GraphTabId;
    static const FName DetailsTabId;

    TObjectPtr<UActionCombatStyleData> StyleData = nullptr;
    TSharedPtr<IDetailsView> DetailsView;
    TSharedPtr<SActionCombatStyleGraphView> GraphView;
    TSharedPtr<class FWorkspaceItem> WorkspaceMenuCategory;
};
