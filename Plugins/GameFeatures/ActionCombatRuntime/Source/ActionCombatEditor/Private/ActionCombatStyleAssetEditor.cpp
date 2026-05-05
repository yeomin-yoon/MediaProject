#include "ActionCombatStyleAssetEditor.h"

#include "ActionCombatStyleData.h"
#include "SActionCombatStyleGraphView.h"

#include "IDetailsView.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "Widgets/Docking/SDockTab.h"

#define LOCTEXT_NAMESPACE "ActionCombatStyleAssetEditor"

const FName FActionCombatStyleAssetEditor::GraphTabId(TEXT("ActionCombatStyleAssetEditor_Graph"));
const FName FActionCombatStyleAssetEditor::DetailsTabId(TEXT("ActionCombatStyleAssetEditor_Details"));

void FActionCombatStyleAssetEditor::InitStyleAssetEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UActionCombatStyleData* InStyleData)
{
    check(InStyleData);

    StyleData = InStyleData;

    FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

    FDetailsViewArgs DetailsArgs;
    DetailsArgs.bAllowSearch = true;
    DetailsArgs.bHideSelectionTip = true;
    DetailsArgs.NameAreaSettings = FDetailsViewArgs::ObjectsUseNameArea;

    DetailsView = PropertyEditorModule.CreateDetailView(DetailsArgs);
    DetailsView->SetObject(StyleData);
    DetailsView->OnFinishedChangingProperties().AddRaw(this, &FActionCombatStyleAssetEditor::HandleFinishedChangingProperties);

    const TSharedRef<FTabManager::FLayout> Layout = FTabManager::NewLayout("ActionCombatStyleAssetEditorLayout_v1")
        ->AddArea
        (
            FTabManager::NewPrimaryArea()->SetOrientation(Orient_Horizontal)
            ->Split
            (
                FTabManager::NewStack()
                ->SetSizeCoefficient(0.72f)
                ->AddTab(GraphTabId, ETabState::OpenedTab)
                ->SetHideTabWell(true)
            )
            ->Split
            (
                FTabManager::NewStack()
                ->SetSizeCoefficient(0.28f)
                ->AddTab(DetailsTabId, ETabState::OpenedTab)
                ->SetHideTabWell(true)
            )
        );

    const TArray<UObject*> ObjectsToEdit = { StyleData };

    InitAssetEditor(Mode, InitToolkitHost, TEXT("ActionCombatStyleAssetEditor"), Layout, true, true, ObjectsToEdit);
    RegenerateMenusAndToolbars();
}

FName FActionCombatStyleAssetEditor::GetToolkitFName() const
{
    return TEXT("ActionCombatStyleAssetEditor");
}

FText FActionCombatStyleAssetEditor::GetBaseToolkitName() const
{
    return LOCTEXT("AppLabel", "Action Combat Style");
}

FText FActionCombatStyleAssetEditor::GetToolkitName() const
{
    const FText BaseName = GetBaseToolkitName();
    if (!StyleData)
    {
        return BaseName;
    }

    return FText::Format(LOCTEXT("ToolkitNameFormat", "{0} - {1}"), BaseName, FText::FromString(StyleData->GetName()));
}

FString FActionCombatStyleAssetEditor::GetWorldCentricTabPrefix() const
{
    return TEXT("ActionCombatStyle");
}

FLinearColor FActionCombatStyleAssetEditor::GetWorldCentricTabColorScale() const
{
    return FLinearColor(0.82f, 0.44f, 0.22f, 1.0f);
}

void FActionCombatStyleAssetEditor::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
    FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

    WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(LOCTEXT("WorkspaceMenuCategory", "Action Combat Style"));
    const TSharedRef<FWorkspaceItem> WorkspaceCategoryRef = WorkspaceMenuCategory.ToSharedRef();

    InTabManager->RegisterTabSpawner(GraphTabId, FOnSpawnTab::CreateRaw(this, &FActionCombatStyleAssetEditor::SpawnGraphTab))
        .SetDisplayName(LOCTEXT("GraphTabLabel", "Combo Graph"))
        .SetGroup(WorkspaceCategoryRef);

    InTabManager->RegisterTabSpawner(DetailsTabId, FOnSpawnTab::CreateRaw(this, &FActionCombatStyleAssetEditor::SpawnDetailsTab))
        .SetDisplayName(LOCTEXT("DetailsTabLabel", "Details"))
        .SetGroup(WorkspaceCategoryRef);
}

void FActionCombatStyleAssetEditor::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
    FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);

    InTabManager->UnregisterTabSpawner(GraphTabId);
    InTabManager->UnregisterTabSpawner(DetailsTabId);
}

void FActionCombatStyleAssetEditor::AddReferencedObjects(FReferenceCollector& Collector)
{
    Collector.AddReferencedObject(StyleData);
}

FString FActionCombatStyleAssetEditor::GetReferencerName() const
{
    return TEXT("FActionCombatStyleAssetEditor");
}

TSharedRef<SDockTab> FActionCombatStyleAssetEditor::SpawnGraphTab(const FSpawnTabArgs& Args)
{
    check(Args.GetTabId() == GraphTabId);

    return SNew(SDockTab)
        .Label(LOCTEXT("GraphTabTitle", "Combo Graph"))
        [
            SAssignNew(GraphView, SActionCombatStyleGraphView)
            .StyleData(StyleData)
            .OnStyleDataEdited(FSimpleDelegate::CreateRaw(this, &FActionCombatStyleAssetEditor::HandleGraphEdited))
        ];
}

TSharedRef<SDockTab> FActionCombatStyleAssetEditor::SpawnDetailsTab(const FSpawnTabArgs& Args)
{
    check(Args.GetTabId() == DetailsTabId);

    return SNew(SDockTab)
        .Label(LOCTEXT("DetailsTabTitle", "Details"))
        [
            DetailsView.ToSharedRef()
        ];
}

void FActionCombatStyleAssetEditor::HandleFinishedChangingProperties(const FPropertyChangedEvent& PropertyChangedEvent)
{
    if (GraphView.IsValid())
    {
        GraphView->RefreshGraph();
    }
}

void FActionCombatStyleAssetEditor::HandleGraphEdited()
{
    if (DetailsView.IsValid())
    {
        DetailsView->ForceRefresh();
    }
}

#undef LOCTEXT_NAMESPACE
