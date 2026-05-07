#pragma once

#include "GraphEditor.h"
#include "GameplayTagContainer.h"
#include "Widgets/SCompoundWidget.h"

class SGraphEditor;
class UActionCombatStyleGraph;
class UActionCombatStyleData;
class UAnimMontage;
struct FActionCombatActionDefinition;
struct FActionCombatTransitionDefinition;

class SActionCombatStyleGraphView : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SActionCombatStyleGraphView)
    {
    }

        SLATE_ARGUMENT(UActionCombatStyleData*, StyleData)
        SLATE_EVENT(FSimpleDelegate, OnStyleDataEdited)

    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);
    virtual ~SActionCombatStyleGraphView() override;
    void RefreshGraph();

private:
    void HandleGraphEdited();
    void HandleGraphDataEdited();
    void HandleGraphSelectionChanged(const FGraphPanelSelectionSet& NewSelection);
    void CaptureGraphNodePositions();
    void UpdateSummaryText();
    void SelectAction(int32 ActionIndex);
    void SelectTransition(int32 TransitionIndex);
    void AddTransition(FGameplayTag FromActionTag, FGameplayTag ToActionTag);
    void AssignMontage(int32 ActionIndex, UAnimMontage* Montage);
    void RefreshSelectionEditor();
    TSharedRef<SWidget> BuildSelectionEditor();
    void ModifyAction(int32 ActionIndex, TFunctionRef<void(FActionCombatActionDefinition&)> Edit, const FText& TransactionText);
    void ModifyTransition(int32 TransitionIndex, TFunctionRef<void(FActionCombatTransitionDefinition&)> Edit, const FText& TransactionText);

private:
    TWeakObjectPtr<UActionCombatStyleData> StyleData;
    FSimpleDelegate OnStyleDataEdited;
    UActionCombatStyleGraph* EditorGraph = nullptr;
    TMap<FString, FVector2D> SavedNodePositions;
    int32 SelectedActionIndex = INDEX_NONE;
    int32 SelectedTransitionIndex = INDEX_NONE;
    TSharedPtr<class STextBlock> SummaryTextBlock;
    TSharedPtr<class SBox> GraphHost;
    TSharedPtr<SGraphEditor> GraphEditor;
    TSharedPtr<class SBox> SelectionHost;
};
