#include "ActionCombatStyleGraph.h"

#include "ActionCombatStyleData.h"

#include "Animation/AnimMontage.h"
#include "AssetRegistry/AssetData.h"
#include "ConnectionDrawingPolicy.h"
#include "EdGraph/EdGraphPin.h"
#include "GameplayTagsManager.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "ActionCombatStyleGraph"

namespace ActionCombatStyleGraph
{
    constexpr float ActionColumnSpacing = 520.0f;
    constexpr float ActionRowSpacing = 210.0f;
    constexpr float TransitionYOffset = 130.0f;
    constexpr float RootX = 0.0f;
    constexpr float RootY = 0.0f;

    const FString RootKey = TEXT("__Root__");

    const TArray<FActionCombatActionDefinition>& GetActions(const UActionCombatStyleData* StyleData)
    {
        static const TArray<FActionCombatActionDefinition> EmptyActions;
        static const FArrayProperty* ActionsProperty = FindFProperty<FArrayProperty>(UActionCombatStyleData::StaticClass(), TEXT("Actions"));

        if (!StyleData || !ActionsProperty)
        {
            return EmptyActions;
        }

        const void* ValuePtr = ActionsProperty->ContainerPtrToValuePtr<void>(StyleData);
        return ValuePtr ? *reinterpret_cast<const TArray<FActionCombatActionDefinition>*>(ValuePtr) : EmptyActions;
    }

    TArray<FActionCombatActionDefinition>* GetMutableActions(UActionCombatStyleData* StyleData)
    {
        static const FArrayProperty* ActionsProperty = FindFProperty<FArrayProperty>(UActionCombatStyleData::StaticClass(), TEXT("Actions"));

        if (!StyleData || !ActionsProperty)
        {
            return nullptr;
        }

        void* ValuePtr = ActionsProperty->ContainerPtrToValuePtr<void>(StyleData);
        return ValuePtr ? reinterpret_cast<TArray<FActionCombatActionDefinition>*>(ValuePtr) : nullptr;
    }

    const TArray<FActionCombatTransitionDefinition>& GetTransitions(const UActionCombatStyleData* StyleData)
    {
        static const TArray<FActionCombatTransitionDefinition> EmptyTransitions;
        static const FArrayProperty* TransitionsProperty = FindFProperty<FArrayProperty>(UActionCombatStyleData::StaticClass(), TEXT("Transitions"));

        if (!StyleData || !TransitionsProperty)
        {
            return EmptyTransitions;
        }

        const void* ValuePtr = TransitionsProperty->ContainerPtrToValuePtr<void>(StyleData);
        return ValuePtr ? *reinterpret_cast<const TArray<FActionCombatTransitionDefinition>*>(ValuePtr) : EmptyTransitions;
    }

    TArray<FActionCombatTransitionDefinition>* GetMutableTransitions(UActionCombatStyleData* StyleData)
    {
        static const FArrayProperty* TransitionsProperty = FindFProperty<FArrayProperty>(UActionCombatStyleData::StaticClass(), TEXT("Transitions"));

        if (!StyleData || !TransitionsProperty)
        {
            return nullptr;
        }

        void* ValuePtr = TransitionsProperty->ContainerPtrToValuePtr<void>(StyleData);
        return ValuePtr ? reinterpret_cast<TArray<FActionCombatTransitionDefinition>*>(ValuePtr) : nullptr;
    }

    FString FormatTagShortName(const FGameplayTag& Tag)
    {
        if (!Tag.IsValid())
        {
            return TEXT("Root");
        }

        FString FullTag = Tag.ToString();
        FString Left;
        FString Right;
        while (FullTag.Split(TEXT("."), &Left, &Right, ESearchCase::CaseSensitive, ESearchDir::FromEnd))
        {
            return Right;
        }

        return FullTag;
    }

    FString GetActionKey(const FGameplayTag& ActionTag, int32 ActionIndex)
    {
        if (ActionTag.IsValid())
        {
            return ActionTag.ToString();
        }

        return FString::Printf(TEXT("__InvalidAction_%d"), ActionIndex);
    }

    FString GetActionSubtitle(const FActionCombatActionDefinition& Action)
    {
        const FString QueueWindow = FString::Printf(
            TEXT("Queue %.2f-%.2f | Commit %.2f"),
            Action.QueueWindowStartsAtNormalizedTime,
            Action.QueueWindowClosesAtNormalizedTime,
            Action.ChainCommitAtNormalizedTime);

        if (Action.Montage)
        {
            return FString::Printf(TEXT("%s\n%s"), *QueueWindow, *Action.Montage->GetName());
        }

        return FString::Printf(TEXT("%s\nFallback %.2fs"), *QueueWindow, Action.FallbackDurationSeconds);
    }

    FString FormatCommandKeyHint(const FGameplayTag& CommandTag)
    {
        const FString Command = CommandTag.ToString();

        if (Command == TEXT("Combat.Command.Light"))
        {
            return TEXT("LMB / Primary");
        }

        if (Command == TEXT("Combat.Command.Alt"))
        {
            return TEXT("RMB / Secondary");
        }

        if (Command == TEXT("Combat.Command.Dodge"))
        {
            return TEXT("Shift / Dodge");
        }

        if (Command == TEXT("Combat.Command.FocusEnter"))
        {
            return TEXT("Ctrl Down / Focus");
        }

        if (Command == TEXT("Combat.Command.FocusExit"))
        {
            return TEXT("Ctrl Up / Focus");
        }

        if (Command == TEXT("Combat.Command.GuardHold"))
        {
            return TEXT("RMB Hold / Guard");
        }

        return FString();
    }

    FString FormatTransitionLabel(const FActionCombatTransitionDefinition& Transition)
    {
        TArray<FString> Lines;
        const FString KeyHint = FormatCommandKeyHint(Transition.CommandTag);
        if (!KeyHint.IsEmpty())
        {
            Lines.Add(KeyHint);
        }

        Lines.Add(Transition.CommandTag.IsValid() ? FormatTagShortName(Transition.CommandTag) : TEXT("Invalid Command"));

        if (Transition.bRequiresFocusActive)
        {
            Lines.Add(TEXT("Focus Active"));
        }

        if (Transition.bRequiresFocusInactive)
        {
            Lines.Add(TEXT("Focus Inactive"));
        }

        if (!Transition.RequiredHeldInputTags.IsEmpty())
        {
            Lines.Add(TEXT("Requires Held"));
        }

        if (!Transition.BlockedHeldInputTags.IsEmpty())
        {
            Lines.Add(TEXT("Blocks Held"));
        }

        return FString::Join(Lines, TEXT("\n"));
    }

    FGameplayTag RequestTagOrNone(const FString& TagString)
    {
        const FString Trimmed = TagString.TrimStartAndEnd();
        if (Trimmed.IsEmpty())
        {
            return FGameplayTag();
        }

        return UGameplayTagsManager::Get().RequestGameplayTag(FName(*Trimmed), false);
    }

    UAnimMontage* GetFirstDroppedMontage(const TArray<FAssetData>& Assets)
    {
        for (const FAssetData& AssetData : Assets)
        {
            if (UAnimMontage* Montage = Cast<UAnimMontage>(AssetData.GetAsset()))
            {
                return Montage;
            }
        }

        return nullptr;
    }
}

class FActionCombatStyleConnectionDrawingPolicy : public FConnectionDrawingPolicy
{
public:
    FActionCombatStyleConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float InZoomFactor, const FSlateRect& InClippingRect, FSlateWindowElementList& InDrawElements)
        : FConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, InZoomFactor, InClippingRect, InDrawElements)
    {
    }

    virtual void DetermineWiringStyle(UEdGraphPin* OutputPin, UEdGraphPin* InputPin, FConnectionParams& Params) override
    {
        FConnectionDrawingPolicy::DetermineWiringStyle(OutputPin, InputPin, Params);

        Params.AssociatedPin1 = OutputPin;
        Params.AssociatedPin2 = InputPin;
        Params.WireThickness = 2.5f;
        Params.WireColor = FLinearColor(0.90f, 0.57f, 0.25f, 1.0f);

        const bool bHasSelectedNode = SelectedGraphNodes.Contains(OutputPin ? OutputPin->GetOwningNode() : nullptr)
            || SelectedGraphNodes.Contains(InputPin ? InputPin->GetOwningNode() : nullptr);
        if (bHasSelectedNode)
        {
            Params.WireThickness = 4.0f;
            Params.WireColor = FLinearColor(1.0f, 0.76f, 0.22f, 1.0f);
        }

        if (!HoveredPins.IsEmpty())
        {
            ApplyHoverDeemphasis(OutputPin, InputPin, Params.WireThickness, Params.WireColor);
        }
    }
};

const FName UActionCombatStyleActionGraphNode::InputPinName(TEXT("In"));
const FName UActionCombatStyleActionGraphNode::OutputPinName(TEXT("Out"));
const FName UActionCombatStyleTransitionGraphNode::InputPinName(TEXT("In"));
const FName UActionCombatStyleTransitionGraphNode::OutputPinName(TEXT("Out"));
const FName UActionCombatStyleGraphSchema::PC_Flow(TEXT("ActionCombatFlow"));

void UActionCombatStyleGraph::Initialize(UActionCombatStyleData* InStyleData)
{
    StyleData = InStyleData;
    Schema = UActionCombatStyleGraphSchema::StaticClass();
}

void UActionCombatStyleGraph::CaptureNodePositions(TMap<FString, FVector2D>& OutNodePositions) const
{
    for (UEdGraphNode* Node : Nodes)
    {
        if (const UActionCombatStyleActionGraphNode* ActionNode = Cast<UActionCombatStyleActionGraphNode>(Node))
        {
            OutNodePositions.Add(ActionNode->GetStableKey(), FVector2D(ActionNode->NodePosX, ActionNode->NodePosY));
        }
        else if (const UActionCombatStyleTransitionGraphNode* TransitionNode = Cast<UActionCombatStyleTransitionGraphNode>(Node))
        {
            OutNodePositions.Add(TransitionNode->GetStableKey(), FVector2D(TransitionNode->NodePosX, TransitionNode->NodePosY));
        }
    }
}

void UActionCombatStyleGraph::RebuildFromStyleData(const TMap<FString, FVector2D>& SavedNodePositions)
{
    using namespace ActionCombatStyleGraph;

    Modify();
    Nodes.Reset();

    ActionCount = 0;
    TransitionCount = 0;
    ReachableActionNodeCount = 0;
    MissingDefinitionCount = 0;

    const TArray<FActionCombatActionDefinition>& Actions = GetActions(StyleData);
    const TArray<FActionCombatTransitionDefinition>& Transitions = GetTransitions(StyleData);

    ActionCount = Actions.Num();
    TransitionCount = Transitions.Num();

    TMap<FString, int32> ActionIndexByKey;
    for (int32 ActionIndex = 0; ActionIndex < Actions.Num(); ++ActionIndex)
    {
        ActionIndexByKey.Add(GetActionKey(Actions[ActionIndex].ActionTag, ActionIndex), ActionIndex);
    }

    TMap<FString, TArray<FString>> Adjacency;
    for (const FActionCombatTransitionDefinition& Transition : Transitions)
    {
        const FString FromKey = Transition.FromActionTag.IsValid() ? Transition.FromActionTag.ToString() : RootKey;
        const FString ToKey = Transition.ToActionTag.IsValid() ? Transition.ToActionTag.ToString() : FString();
        if (!ToKey.IsEmpty())
        {
            Adjacency.FindOrAdd(FromKey).Add(ToKey);
        }
    }

    TMap<FString, int32> DepthByKey;
    TMap<FString, int32> RowByKey;
    TMap<int32, TSet<int32>> UsedRowsByDepth;

    auto ReserveRow = [&UsedRowsByDepth](int32 Depth, int32 PreferredRow)
    {
        TSet<int32>& UsedRows = UsedRowsByDepth.FindOrAdd(Depth);
        int32 Row = FMath::Max(PreferredRow, 0);
        while (UsedRows.Contains(Row))
        {
            ++Row;
        }

        UsedRows.Add(Row);
        return Row;
    };

    TArray<FString> VisitQueue;
    VisitQueue.Add(RootKey);
    DepthByKey.Add(RootKey, 0);
    RowByKey.Add(RootKey, ReserveRow(0, 0));

    int32 NextBranchRow = 1;
    for (int32 QueueIndex = 0; QueueIndex < VisitQueue.Num(); ++QueueIndex)
    {
        const FString CurrentKey = VisitQueue[QueueIndex];
        const int32 ParentDepth = DepthByKey.FindChecked(CurrentKey);
        const int32 ParentRow = RowByKey.FindChecked(CurrentKey);
        int32 ChildSlot = 0;

        if (const TArray<FString>* Children = Adjacency.Find(CurrentKey))
        {
            for (const FString& ChildKey : *Children)
            {
                if (DepthByKey.Contains(ChildKey))
                {
                    ++ChildSlot;
                    continue;
                }

                const int32 ChildDepth = ParentDepth + 1;
                const int32 PreferredRow = ChildSlot == 0 ? ParentRow : NextBranchRow++;
                const int32 ChildRow = ReserveRow(ChildDepth, PreferredRow);
                DepthByKey.Add(ChildKey, ChildDepth);
                RowByKey.Add(ChildKey, ChildRow);
                NextBranchRow = FMath::Max(NextBranchRow, ChildRow + 1);
                VisitQueue.Add(ChildKey);
                ++ChildSlot;
            }
        }
    }

    int32 MaxReachableDepth = 0;
    for (const TPair<FString, int32>& Pair : DepthByKey)
    {
        MaxReachableDepth = FMath::Max(MaxReachableDepth, Pair.Value);
        if (Pair.Key != RootKey && ActionIndexByKey.Contains(Pair.Key))
        {
            ++ReachableActionNodeCount;
        }
    }

    TMap<FString, UActionCombatStyleActionGraphNode*> ActionNodesByKey;
    UActionCombatStyleActionGraphNode* RootNode = CreateActionVisualNode(INDEX_NONE, FGameplayTag(), true, false, FVector2D(RootX, RootY));
    ActionNodesByKey.Add(RootKey, RootNode);

    for (int32 ActionIndex = 0; ActionIndex < Actions.Num(); ++ActionIndex)
    {
        const FActionCombatActionDefinition& Action = Actions[ActionIndex];
        const FString ActionKey = GetActionKey(Action.ActionTag, ActionIndex);

        if (!DepthByKey.Contains(ActionKey))
        {
            const int32 UnreachableDepth = MaxReachableDepth + 1;
            DepthByKey.Add(ActionKey, UnreachableDepth);
            RowByKey.Add(ActionKey, ReserveRow(UnreachableDepth, NextBranchRow++));
        }

        const int32 Depth = DepthByKey.FindChecked(ActionKey);
        const int32 Row = RowByKey.FindChecked(ActionKey);
        FVector2D Position(Depth * ActionColumnSpacing, Row * ActionRowSpacing);
        if (const FVector2D* SavedPosition = SavedNodePositions.Find(ActionKey))
        {
            Position = *SavedPosition;
        }

        UActionCombatStyleActionGraphNode* Node = CreateActionVisualNode(ActionIndex, Action.ActionTag, false, false, Position);
        Node->Subtitle = GetActionSubtitle(Action);
        ActionNodesByKey.Add(ActionKey, Node);
    }

    for (const FActionCombatTransitionDefinition& Transition : Transitions)
    {
        if (!Transition.ToActionTag.IsValid())
        {
            continue;
        }

        const FString ToKey = Transition.ToActionTag.ToString();
        if (!ActionNodesByKey.Contains(ToKey))
        {
            const int32 Depth = DepthByKey.Contains(ToKey) ? DepthByKey.FindChecked(ToKey) : MaxReachableDepth + 1;
            const int32 Row = RowByKey.Contains(ToKey) ? RowByKey.FindChecked(ToKey) : ReserveRow(Depth, NextBranchRow++);
            UActionCombatStyleActionGraphNode* MissingNode = CreateActionVisualNode(INDEX_NONE, Transition.ToActionTag, false, true, FVector2D(Depth * ActionColumnSpacing, Row * ActionRowSpacing));
            MissingNode->Subtitle = TEXT("Referenced by a transition,\nbut no action entry exists.");
            ActionNodesByKey.Add(ToKey, MissingNode);
            ++MissingDefinitionCount;
        }
    }

    TArray<UActionCombatStyleTransitionGraphNode*> TransitionNodes;
    for (int32 TransitionIndex = 0; TransitionIndex < Transitions.Num(); ++TransitionIndex)
    {
        const FActionCombatTransitionDefinition& Transition = Transitions[TransitionIndex];
        const FString FromKey = Transition.FromActionTag.IsValid() ? Transition.FromActionTag.ToString() : RootKey;
        const FString ToKey = Transition.ToActionTag.IsValid() ? Transition.ToActionTag.ToString() : FString();

        UActionCombatStyleActionGraphNode* const* FromNodePtr = ActionNodesByKey.Find(FromKey);
        UActionCombatStyleActionGraphNode* const* ToNodePtr = ActionNodesByKey.Find(ToKey);
        if (!FromNodePtr || !ToNodePtr)
        {
            continue;
        }

        const FString TransitionKey = FString::Printf(TEXT("__Transition_%d"), TransitionIndex);
        FVector2D Position(
            ((*FromNodePtr)->NodePosX + (*ToNodePtr)->NodePosX) * 0.5f,
            ((*FromNodePtr)->NodePosY + (*ToNodePtr)->NodePosY) * 0.5f + TransitionYOffset);

        if (const FVector2D* SavedPosition = SavedNodePositions.Find(TransitionKey))
        {
            Position = *SavedPosition;
        }

        UActionCombatStyleTransitionGraphNode* TransitionNode = CreateTransitionVisualNode(TransitionIndex, Position);
        TransitionNode->Label = FormatTransitionLabel(Transition);
        TransitionNode->FromActionTag = Transition.FromActionTag;
        TransitionNode->ToActionTag = Transition.ToActionTag;
        TransitionNodes.Add(TransitionNode);
    }

    RebuildPinsAndLinks(ActionNodesByKey, TransitionNodes);
    NotifyGraphChanged();
}

UActionCombatStyleActionGraphNode* UActionCombatStyleGraph::CreateActionVisualNode(int32 ActionIndex, const FGameplayTag& ActionTag, bool bRootNode, bool bMissingDefinition, const FVector2D& GraphPosition)
{
    UActionCombatStyleActionGraphNode* Node = NewObject<UActionCombatStyleActionGraphNode>(this);
    Node->SetFlags(RF_Transactional);
    Node->ActionIndex = ActionIndex;
    Node->ActionTag = ActionTag;
    Node->bRootNode = bRootNode;
    Node->bMissingDefinition = bMissingDefinition;
    Node->NodePosX = static_cast<int32>(GraphPosition.X);
    Node->NodePosY = static_cast<int32>(GraphPosition.Y);
    AddNode(Node, false, false);
    Node->CreateNewGuid();
    Node->AllocateDefaultPins();
    return Node;
}

UActionCombatStyleTransitionGraphNode* UActionCombatStyleGraph::CreateTransitionVisualNode(int32 TransitionIndex, const FVector2D& GraphPosition)
{
    UActionCombatStyleTransitionGraphNode* Node = NewObject<UActionCombatStyleTransitionGraphNode>(this);
    Node->SetFlags(RF_Transactional);
    Node->TransitionIndex = TransitionIndex;
    Node->NodePosX = static_cast<int32>(GraphPosition.X);
    Node->NodePosY = static_cast<int32>(GraphPosition.Y);
    AddNode(Node, false, false);
    Node->CreateNewGuid();
    Node->AllocateDefaultPins();
    return Node;
}

void UActionCombatStyleGraph::RebuildPinsAndLinks(const TMap<FString, UActionCombatStyleActionGraphNode*>& ActionNodesByKey, const TArray<UActionCombatStyleTransitionGraphNode*>& TransitionNodes)
{
    using namespace ActionCombatStyleGraph;

    const TArray<FActionCombatTransitionDefinition>& Transitions = GetTransitions(StyleData);
    for (UActionCombatStyleTransitionGraphNode* TransitionNode : TransitionNodes)
    {
        if (!TransitionNode || !Transitions.IsValidIndex(TransitionNode->TransitionIndex))
        {
            continue;
        }

        const FActionCombatTransitionDefinition& Transition = Transitions[TransitionNode->TransitionIndex];
        const FString FromKey = Transition.FromActionTag.IsValid() ? Transition.FromActionTag.ToString() : RootKey;
        const FString ToKey = Transition.ToActionTag.IsValid() ? Transition.ToActionTag.ToString() : FString();

        UActionCombatStyleActionGraphNode* const* FromNodePtr = ActionNodesByKey.Find(FromKey);
        UActionCombatStyleActionGraphNode* const* ToNodePtr = ActionNodesByKey.Find(ToKey);
        if (!FromNodePtr || !ToNodePtr)
        {
            continue;
        }

        if (UEdGraphPin* FromOutput = (*FromNodePtr)->GetOutputPin())
        {
            FromOutput->MakeLinkTo(TransitionNode->GetInputPin());
        }

        if (UEdGraphPin* TransitionOutput = TransitionNode->GetOutputPin())
        {
            TransitionOutput->MakeLinkTo((*ToNodePtr)->GetInputPin());
        }
    }
}

UActionCombatStyleActionGraphNode* UActionCombatStyleGraph::AddActionNodeAt(const FVector2D& GraphPosition)
{
    using namespace ActionCombatStyleGraph;

    UActionCombatStyleData* Data = StyleData.Get();
    TArray<FActionCombatActionDefinition>* Actions = GetMutableActions(Data);
    if (!Data || !Actions)
    {
        return nullptr;
    }

    const FScopedTransaction Transaction(LOCTEXT("AddActionNodeTransaction", "Add Action Combat Action Node"));
    Data->Modify();
    Modify();

    FActionCombatActionDefinition NewAction;
    if (!Actions->IsEmpty())
    {
        NewAction = Actions->Last();
        NewAction.ActionTag = FGameplayTag();
        NewAction.Montage = nullptr;
    }

    const int32 NewActionIndex = Actions->Add(NewAction);
    Data->MarkPackageDirty();
    Data->PostEditChange();

    UActionCombatStyleActionGraphNode* NewNode = CreateActionVisualNode(NewActionIndex, FGameplayTag(), false, false, GraphPosition);
    NewNode->Subtitle = GetActionSubtitle(NewAction);
    ++ActionCount;

    NotifyGraphChanged();
    NotifyStyleDataEdited();
    return NewNode;
}

UActionCombatStyleTransitionGraphNode* UActionCombatStyleGraph::AddTransitionNodeBetween(UActionCombatStyleActionGraphNode* FromNode, UActionCombatStyleActionGraphNode* ToNode, const FVector2D& GraphPosition)
{
    using namespace ActionCombatStyleGraph;

    UActionCombatStyleData* Data = StyleData.Get();
    TArray<FActionCombatTransitionDefinition>* Transitions = GetMutableTransitions(Data);
    if (!Data || !Transitions || !FromNode || !ToNode || !ToNode->ActionTag.IsValid())
    {
        return nullptr;
    }

    if (!FromNode->bRootNode && !FromNode->ActionTag.IsValid())
    {
        return nullptr;
    }

    const FScopedTransaction Transaction(LOCTEXT("AddTransitionNodeTransaction", "Add Action Combat Transition"));
    Data->Modify();
    Modify();

    FActionCombatTransitionDefinition NewTransition;
    NewTransition.FromActionTag = FromNode->bRootNode ? FGameplayTag() : FromNode->ActionTag;
    NewTransition.ToActionTag = ToNode->ActionTag;
    NewTransition.CommandTag = RequestTagOrNone(TEXT("Combat.Command.Light"));

    const int32 NewTransitionIndex = Transitions->Add(NewTransition);
    Data->MarkPackageDirty();
    Data->PostEditChange();

    UActionCombatStyleTransitionGraphNode* TransitionNode = CreateTransitionVisualNode(NewTransitionIndex, GraphPosition);
    TransitionNode->Label = FormatTransitionLabel(NewTransition);
    TransitionNode->FromActionTag = NewTransition.FromActionTag;
    TransitionNode->ToActionTag = NewTransition.ToActionTag;

    if (UEdGraphPin* FromOutput = FromNode->GetOutputPin())
    {
        FromOutput->MakeLinkTo(TransitionNode->GetInputPin());
    }

    if (UEdGraphPin* TransitionOutput = TransitionNode->GetOutputPin())
    {
        TransitionOutput->MakeLinkTo(ToNode->GetInputPin());
    }

    ++TransitionCount;
    NotifyGraphChanged();
    NotifyStyleDataEdited();
    return TransitionNode;
}

bool UActionCombatStyleGraph::AssignMontageToActionNode(UActionCombatStyleActionGraphNode* ActionNode, UAnimMontage* Montage)
{
    using namespace ActionCombatStyleGraph;

    UActionCombatStyleData* Data = StyleData.Get();
    TArray<FActionCombatActionDefinition>* Actions = GetMutableActions(Data);
    if (!Data || !Actions || !ActionNode || !Actions->IsValidIndex(ActionNode->ActionIndex))
    {
        return false;
    }

    const FScopedTransaction Transaction(LOCTEXT("AssignMontageTransaction", "Assign Action Combat Montage"));
    Data->Modify();
    ActionNode->Modify();

    (*Actions)[ActionNode->ActionIndex].Montage = Montage;
    ActionNode->Subtitle = GetActionSubtitle((*Actions)[ActionNode->ActionIndex]);

    Data->MarkPackageDirty();
    Data->PostEditChange();
    NotifyGraphChanged();
    NotifyStyleDataEdited();
    return true;
}

void UActionCombatStyleGraph::NotifyStyleDataEdited()
{
    if (OnStyleDataEdited.IsBound())
    {
        OnStyleDataEdited.Execute();
    }
}

void UActionCombatStyleActionGraphNode::AllocateDefaultPins()
{
    if (!bRootNode)
    {
        CreatePin(EGPD_Input, UActionCombatStyleGraphSchema::PC_Flow, InputPinName);
    }

    CreatePin(EGPD_Output, UActionCombatStyleGraphSchema::PC_Flow, OutputPinName);
}

FText UActionCombatStyleActionGraphNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
    if (bRootNode)
    {
        return LOCTEXT("RootNodeTitle", "Root\nNeutral Entry");
    }

    const FString Title = ActionTag.IsValid() ? ActionCombatStyleGraph::FormatTagShortName(ActionTag) : TEXT("New Action");
    if (!Subtitle.IsEmpty() && TitleType != ENodeTitleType::ListView)
    {
        return FText::FromString(Title + TEXT("\n") + Subtitle);
    }

    return FText::FromString(Title);
}

FText UActionCombatStyleActionGraphNode::GetTooltipText() const
{
    if (bRootNode)
    {
        return LOCTEXT("RootNodeTooltip", "Neutral-start transitions begin here.");
    }

    if (bMissingDefinition)
    {
        return LOCTEXT("MissingDefinitionTooltip", "A transition references this action tag, but the style data has no matching action entry.");
    }

    return FText::FromString(Subtitle);
}

FLinearColor UActionCombatStyleActionGraphNode::GetNodeTitleColor() const
{
    if (bRootNode)
    {
        return FLinearColor(0.18f, 0.34f, 0.40f, 1.0f);
    }

    if (bMissingDefinition)
    {
        return FLinearColor(0.52f, 0.16f, 0.14f, 1.0f);
    }

    if (!ActionTag.IsValid())
    {
        return FLinearColor(0.36f, 0.28f, 0.12f, 1.0f);
    }

    return FLinearColor(0.18f, 0.20f, 0.25f, 1.0f);
}

FLinearColor UActionCombatStyleActionGraphNode::GetNodeBodyTintColor() const
{
    return GetNodeTitleColor();
}

bool UActionCombatStyleActionGraphNode::CanUserDeleteNode() const
{
    return false;
}

UEdGraphPin* UActionCombatStyleActionGraphNode::GetInputPin() const
{
    return FindPin(InputPinName, EGPD_Input);
}

UEdGraphPin* UActionCombatStyleActionGraphNode::GetOutputPin() const
{
    return FindPin(OutputPinName, EGPD_Output);
}

FString UActionCombatStyleActionGraphNode::GetStableKey() const
{
    if (bRootNode)
    {
        return ActionCombatStyleGraph::RootKey;
    }

    return ActionCombatStyleGraph::GetActionKey(ActionTag, ActionIndex);
}

void UActionCombatStyleTransitionGraphNode::AllocateDefaultPins()
{
    CreatePin(EGPD_Input, UActionCombatStyleGraphSchema::PC_Flow, InputPinName);
    CreatePin(EGPD_Output, UActionCombatStyleGraphSchema::PC_Flow, OutputPinName);
}

FText UActionCombatStyleTransitionGraphNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
    return FText::FromString(Label.IsEmpty() ? TEXT("Transition") : Label);
}

FText UActionCombatStyleTransitionGraphNode::GetTooltipText() const
{
    return FText::Format(
        LOCTEXT("TransitionNodeTooltip", "Transition {0}\nFrom: {1}\nTo: {2}"),
        TransitionIndex,
        FText::FromString(FromActionTag.IsValid() ? FromActionTag.ToString() : TEXT("Root")),
        FText::FromString(ToActionTag.IsValid() ? ToActionTag.ToString() : TEXT("Invalid")));
}

FLinearColor UActionCombatStyleTransitionGraphNode::GetNodeTitleColor() const
{
    return FLinearColor(0.43f, 0.24f, 0.09f, 1.0f);
}

FLinearColor UActionCombatStyleTransitionGraphNode::GetNodeBodyTintColor() const
{
    return FLinearColor(0.25f, 0.17f, 0.10f, 1.0f);
}

bool UActionCombatStyleTransitionGraphNode::CanUserDeleteNode() const
{
    return false;
}

UEdGraphPin* UActionCombatStyleTransitionGraphNode::GetInputPin() const
{
    return FindPin(InputPinName, EGPD_Input);
}

UEdGraphPin* UActionCombatStyleTransitionGraphNode::GetOutputPin() const
{
    return FindPin(OutputPinName, EGPD_Output);
}

FString UActionCombatStyleTransitionGraphNode::GetStableKey() const
{
    return FString::Printf(TEXT("__Transition_%d"), TransitionIndex);
}

UEdGraphNode* FActionCombatGraphSchemaAction_AddActionNode::PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode)
{
    UActionCombatStyleGraph* StyleGraph = Cast<UActionCombatStyleGraph>(ParentGraph);
    if (!StyleGraph)
    {
        return nullptr;
    }

    UActionCombatStyleActionGraphNode* NewNode = StyleGraph->AddActionNodeAt(Location);
    if (FromPin && NewNode && NewNode->GetInputPin())
    {
        GetDefault<UActionCombatStyleGraphSchema>()->TryCreateConnection(FromPin, NewNode->GetInputPin());
    }

    return NewNode;
}

void UActionCombatStyleGraphSchema::GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
    const TSharedPtr<FActionCombatGraphSchemaAction_AddActionNode> AddAction(new FActionCombatGraphSchemaAction_AddActionNode(
        LOCTEXT("ActionCombatCategory", "Action Combat"),
        LOCTEXT("AddActionNode", "Add Action Node"),
        LOCTEXT("AddActionNodeTooltip", "Append a new action entry to this style data."),
        0));
    ContextMenuBuilder.AddAction(AddAction);
}

const FPinConnectionResponse UActionCombatStyleGraphSchema::CanCreateConnection(const UEdGraphPin* PinA, const UEdGraphPin* PinB) const
{
    if (!PinA || !PinB)
    {
        return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Invalid pins"));
    }

    if (PinA->GetOwningNode() == PinB->GetOwningNode())
    {
        return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Cannot connect a node to itself"));
    }

    if (PinA->Direction == PinB->Direction)
    {
        return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Pins must have opposite directions"));
    }

    const UEdGraphPin* OutputPin = PinA->Direction == EGPD_Output ? PinA : PinB;
    const UEdGraphPin* InputPin = PinA->Direction == EGPD_Input ? PinA : PinB;
    const UActionCombatStyleActionGraphNode* FromAction = Cast<UActionCombatStyleActionGraphNode>(OutputPin->GetOwningNode());
    const UActionCombatStyleActionGraphNode* ToAction = Cast<UActionCombatStyleActionGraphNode>(InputPin->GetOwningNode());

    if (FromAction && ToAction)
    {
        if (!ToAction->ActionTag.IsValid())
        {
            return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Set the target ActionTag before creating a transition"));
        }

        if (!FromAction->bRootNode && !FromAction->ActionTag.IsValid())
        {
            return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Set the source ActionTag before creating a transition"));
        }

        return FPinConnectionResponse(CONNECT_RESPONSE_MAKE_WITH_CONVERSION_NODE, TEXT("Create transition"));
    }

    return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Drag from an action output pin to another action input pin"));
}

bool UActionCombatStyleGraphSchema::TryCreateConnection(UEdGraphPin* PinA, UEdGraphPin* PinB) const
{
    if (!PinA || !PinB)
    {
        return false;
    }

    const FPinConnectionResponse Response = CanCreateConnection(PinA, PinB);
    if (Response.Response == CONNECT_RESPONSE_MAKE_WITH_CONVERSION_NODE)
    {
        return CreateAutomaticConversionNodeAndConnections(PinA, PinB);
    }

    return false;
}

bool UActionCombatStyleGraphSchema::CreateAutomaticConversionNodeAndConnections(UEdGraphPin* PinA, UEdGraphPin* PinB) const
{
    if (!PinA || !PinB)
    {
        return false;
    }

    UEdGraphPin* OutputPin = PinA->Direction == EGPD_Output ? PinA : PinB;
    UEdGraphPin* InputPin = PinA->Direction == EGPD_Input ? PinA : PinB;
    UActionCombatStyleActionGraphNode* FromAction = Cast<UActionCombatStyleActionGraphNode>(OutputPin->GetOwningNode());
    UActionCombatStyleActionGraphNode* ToAction = Cast<UActionCombatStyleActionGraphNode>(InputPin->GetOwningNode());
    UActionCombatStyleGraph* StyleGraph = FromAction ? Cast<UActionCombatStyleGraph>(FromAction->GetGraph()) : nullptr;
    if (!StyleGraph || !FromAction || !ToAction)
    {
        return false;
    }

    const FVector2D TransitionPosition(
        (FromAction->NodePosX + ToAction->NodePosX) * 0.5f,
        (FromAction->NodePosY + ToAction->NodePosY) * 0.5f + ActionCombatStyleGraph::TransitionYOffset);
    return StyleGraph->AddTransitionNodeBetween(FromAction, ToAction, TransitionPosition) != nullptr;
}

FLinearColor UActionCombatStyleGraphSchema::GetPinTypeColor(const FEdGraphPinType& PinType) const
{
    return FLinearColor(0.90f, 0.57f, 0.25f, 1.0f);
}

EGraphType UActionCombatStyleGraphSchema::GetGraphType(const UEdGraph* TestEdGraph) const
{
    return GT_StateMachine;
}

void UActionCombatStyleGraphSchema::DroppedAssetsOnNode(const TArray<FAssetData>& Assets, const FVector2D& GraphPosition, UEdGraphNode* Node) const
{
    UAnimMontage* Montage = ActionCombatStyleGraph::GetFirstDroppedMontage(Assets);
    UActionCombatStyleActionGraphNode* ActionNode = Cast<UActionCombatStyleActionGraphNode>(Node);
    UActionCombatStyleGraph* StyleGraph = ActionNode ? Cast<UActionCombatStyleGraph>(ActionNode->GetGraph()) : nullptr;
    if (Montage && ActionNode && StyleGraph)
    {
        StyleGraph->AssignMontageToActionNode(ActionNode, Montage);
    }
}

void UActionCombatStyleGraphSchema::GetAssetsNodeHoverMessage(const TArray<FAssetData>& Assets, const UEdGraphNode* HoverNode, FString& OutTooltipText, bool& OutOkIcon) const
{
    const bool bCanAssignMontage = ActionCombatStyleGraph::GetFirstDroppedMontage(Assets) != nullptr
        && Cast<UActionCombatStyleActionGraphNode>(HoverNode) != nullptr;

    OutOkIcon = bCanAssignMontage;
    OutTooltipText = bCanAssignMontage
        ? TEXT("Assign montage to this action")
        : TEXT("");
}

FConnectionDrawingPolicy* UActionCombatStyleGraphSchema::CreateConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float InZoomFactor, const FSlateRect& InClippingRect, FSlateWindowElementList& InDrawElements, UEdGraph* InGraphObj) const
{
    return new FActionCombatStyleConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, InZoomFactor, InClippingRect, InDrawElements);
}

#undef LOCTEXT_NAMESPACE
