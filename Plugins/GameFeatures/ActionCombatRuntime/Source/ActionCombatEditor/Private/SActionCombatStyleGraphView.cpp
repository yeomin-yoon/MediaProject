#include "SActionCombatStyleGraphView.h"

#include "ActionCombatStyleGraph.h"
#include "ActionCombatStyleData.h"

#include "Animation/AnimMontage.h"
#include "DragAndDrop/AssetDragDropOp.h"
#include "DragAndDrop/DecoratedDragDropOp.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "GameplayTagsManager.h"
#include "PropertyCustomizationHelpers.h"
#include "Rendering/DrawElements.h"
#include "ScopedTransaction.h"
#include "Styling/AppStyle.h"
#include "UObject/UnrealType.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SConstraintCanvas.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "SActionCombatStyleGraphView"

namespace ActionCombatStyleGraphView
{
    DECLARE_DELEGATE_OneParam(FOnActionIndex, int32);
    DECLARE_DELEGATE_OneParam(FOnTransitionIndex, int32);
    DECLARE_DELEGATE_TwoParams(FOnTransitionRequested, FGameplayTag, FGameplayTag);
    DECLARE_DELEGATE_TwoParams(FOnMontageDropped, int32, UAnimMontage*);

    constexpr float CanvasMargin = 80.0f;
    constexpr float NodeWidth = 260.0f;
    constexpr float NodeHeight = 112.0f;
    constexpr float ColumnSpacing = 440.0f;
    constexpr float RowSpacing = 190.0f;
    constexpr float LineThickness = 3.0f;
    constexpr float EdgeAnchorGap = 18.0f;
    constexpr float ArrowLength = 20.0f;
    constexpr float ArrowHalfWidth = 10.0f;
    const FVector2D EdgeLabelSize = FVector2D(190.0f, 38.0f);

    struct FGraphNodeVisual
    {
        FString Key;
        FString Title;
        FString Subtitle;
        FGameplayTag ActionTag;
        FVector2D Position = FVector2D::ZeroVector;
        FVector2D Size = FVector2D(NodeWidth, NodeHeight);
        int32 ActionIndex = INDEX_NONE;
        FLinearColor FillColor = FLinearColor(0.17f, 0.20f, 0.24f, 1.0f);
        bool bIsSynthetic = false;
        bool bIsMissingDefinition = false;
    };

    struct FGraphEdgeVisual
    {
        int32 FromNodeIndex = INDEX_NONE;
        int32 ToNodeIndex = INDEX_NONE;
        int32 TransitionIndex = INDEX_NONE;
        FString Label;
        FLinearColor Color = FLinearColor(0.85f, 0.54f, 0.26f, 1.0f);
    };

    struct FGraphLayout
    {
        TArray<FGraphNodeVisual> Nodes;
        TArray<FGraphEdgeVisual> Edges;
        FVector2D CanvasSize = FVector2D(800.0f, 600.0f);
        int32 ReachableActionNodeCount = 0;
        int32 MissingDefinitionCount = 0;
        int32 TransitionCount = 0;
        int32 ActionCount = 0;
    };

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

    FString FormatTag(const FGameplayTag& Tag)
    {
        return Tag.IsValid() ? Tag.ToString() : TEXT("Root");
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

    FString FormatActionSubtitle(const FActionCombatActionDefinition& Action)
    {
        const FString QueueWindowText = FString::Printf(
            TEXT("Queue %.2f-%.2f | Commit %.2f"),
            Action.QueueWindowStartsAtNormalizedTime,
            Action.QueueWindowClosesAtNormalizedTime,
            Action.ChainCommitAtNormalizedTime);

        if (Action.Montage)
        {
            return FString::Printf(TEXT("%s\n%s"), *QueueWindowText, *Action.Montage->GetName());
        }

        return FString::Printf(TEXT("%s\nFallback %.2fs"), *QueueWindowText, Action.FallbackDurationSeconds);
    }

    FString FormatTagContainerList(const FGameplayTagContainer& TagContainer)
    {
        TArray<FString> TagStrings;
        for (const FGameplayTag& Tag : TagContainer)
        {
            TagStrings.Add(Tag.ToString());
        }

        TagStrings.Sort();
        return FString::Join(TagStrings, TEXT(", "));
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
        TArray<FString> Parts;
        const FString KeyHint = FormatCommandKeyHint(Transition.CommandTag);
        if (!KeyHint.IsEmpty())
        {
            Parts.Add(KeyHint);
        }

        Parts.Add(Transition.CommandTag.IsValid() ? FormatTagShortName(Transition.CommandTag) : TEXT("Invalid Command"));

        if (Transition.bRequiresFocusActive)
        {
            Parts.Add(TEXT("Focus On"));
        }

        if (Transition.bRequiresFocusInactive)
        {
            Parts.Add(TEXT("Focus Off"));
        }

        if (!Transition.RequiredHeldInputTags.IsEmpty())
        {
            Parts.Add(FString::Printf(TEXT("+ %s"), *FormatTagContainerList(Transition.RequiredHeldInputTags)));
        }

        if (!Transition.BlockedHeldInputTags.IsEmpty())
        {
            Parts.Add(FString::Printf(TEXT("- %s"), *FormatTagContainerList(Transition.BlockedHeldInputTags)));
        }

        return FString::Join(Parts, TEXT("\n"));
    }

    int32 FindOrAddNode(
        FGraphLayout& Layout,
        TMap<FString, int32>& NodeIndexByKey,
        const FString& Key,
        TFunctionRef<void(FGraphNodeVisual&)> Initializer)
    {
        if (const int32* ExistingIndex = NodeIndexByKey.Find(Key))
        {
            return *ExistingIndex;
        }

        FGraphNodeVisual& Node = Layout.Nodes.AddDefaulted_GetRef();
        Initializer(Node);

        const int32 NewIndex = Layout.Nodes.Num() - 1;
        NodeIndexByKey.Add(Key, NewIndex);
        return NewIndex;
    }

    FGraphLayout BuildGraphLayout(const UActionCombatStyleData* StyleData)
    {
        FGraphLayout Layout;

        const TArray<FActionCombatActionDefinition>& Actions = GetActions(StyleData);
        const TArray<FActionCombatTransitionDefinition>& Transitions = GetTransitions(StyleData);

        Layout.ActionCount = Actions.Num();
        Layout.TransitionCount = Transitions.Num();

        TMap<FString, int32> NodeIndexByKey;
        const int32 RootIndex = FindOrAddNode(
            Layout,
            NodeIndexByKey,
            TEXT("__Root__"),
            [](FGraphNodeVisual& Node)
            {
                Node.Key = TEXT("__Root__");
                Node.Title = TEXT("Root");
                Node.Subtitle = TEXT("Neutral entry");
                Node.FillColor = FLinearColor(0.18f, 0.30f, 0.36f, 1.0f);
                Node.bIsSynthetic = true;
            });

        for (int32 ActionIndex = 0; ActionIndex < Actions.Num(); ++ActionIndex)
        {
            const FActionCombatActionDefinition& Action = Actions[ActionIndex];
            const bool bHasValidActionTag = Action.ActionTag.IsValid();
            const FString ActionKey = bHasValidActionTag
                ? FormatTag(Action.ActionTag)
                : FString::Printf(TEXT("__InvalidAction_%d"), ActionIndex);

            FindOrAddNode(
                Layout,
                NodeIndexByKey,
                ActionKey,
                [&Action, &ActionKey, bHasValidActionTag, ActionIndex](FGraphNodeVisual& Node)
                {
                    Node.Key = ActionKey;
                    Node.Title = bHasValidActionTag ? FormatTagShortName(Action.ActionTag) : TEXT("New Action");
                    Node.Subtitle = FormatActionSubtitle(Action);
                    Node.ActionTag = Action.ActionTag;
                    Node.ActionIndex = ActionIndex;
                    Node.FillColor = bHasValidActionTag
                        ? FLinearColor(0.20f, 0.21f, 0.25f, 1.0f)
                        : FLinearColor(0.31f, 0.25f, 0.12f, 1.0f);
                });
        }

        TArray<TArray<int32>> Adjacency;
        Adjacency.SetNum(Layout.Nodes.Num());

        for (int32 TransitionIndex = 0; TransitionIndex < Transitions.Num(); ++TransitionIndex)
        {
            const FActionCombatTransitionDefinition& Transition = Transitions[TransitionIndex];
            const FString FromKey = Transition.FromActionTag.IsValid() ? Transition.FromActionTag.ToString() : TEXT("__Root__");
            const FString ToKey = FormatTag(Transition.ToActionTag);

            const bool bFromMissing = Transition.FromActionTag.IsValid() && !NodeIndexByKey.Contains(FromKey);
            const bool bToMissing = Transition.ToActionTag.IsValid() && !NodeIndexByKey.Contains(ToKey);

            const int32 FromIndex = FindOrAddNode(
                Layout,
                NodeIndexByKey,
                FromKey,
                [&FromKey](FGraphNodeVisual& Node)
                {
                    Node.Key = FromKey;
                    Node.Title = FromKey;
                    Node.Subtitle = TEXT("Referenced by a transition, but no action entry exists.");
                    Node.FillColor = FLinearColor(0.39f, 0.17f, 0.16f, 1.0f);
                    Node.bIsMissingDefinition = true;
                });

            const int32 ToIndex = FindOrAddNode(
                Layout,
                NodeIndexByKey,
                ToKey,
                [&ToKey](FGraphNodeVisual& Node)
                {
                    Node.Key = ToKey;
                    Node.Title = ToKey;
                    Node.Subtitle = TEXT("Referenced by a transition, but no action entry exists.");
                    Node.FillColor = FLinearColor(0.39f, 0.17f, 0.16f, 1.0f);
                    Node.bIsMissingDefinition = true;
                });

            if (bFromMissing)
            {
                Layout.MissingDefinitionCount++;
            }

            if (bToMissing)
            {
                Layout.MissingDefinitionCount++;
            }

            FGraphEdgeVisual& Edge = Layout.Edges.AddDefaulted_GetRef();
            Edge.FromNodeIndex = FromIndex;
            Edge.ToNodeIndex = ToIndex;
            Edge.TransitionIndex = TransitionIndex;
            Edge.Label = FormatTransitionLabel(Transition);
            Edge.Color = (Layout.Nodes[FromIndex].bIsMissingDefinition || Layout.Nodes[ToIndex].bIsMissingDefinition)
                ? FLinearColor(0.89f, 0.42f, 0.32f, 1.0f)
                : FLinearColor(0.87f, 0.58f, 0.30f, 1.0f);

            if (!Adjacency.IsValidIndex(FromIndex))
            {
                Adjacency.SetNum(Layout.Nodes.Num());
            }

            Adjacency[FromIndex].Add(ToIndex);
        }

        if (Adjacency.Num() < Layout.Nodes.Num())
        {
            Adjacency.SetNum(Layout.Nodes.Num());
        }

        TArray<int32> DepthByNode;
        DepthByNode.Init(INDEX_NONE, Layout.Nodes.Num());
        TArray<int32> RowByNode;
        RowByNode.Init(INDEX_NONE, Layout.Nodes.Num());
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

        TArray<int32> VisitQueue;
        VisitQueue.Add(RootIndex);
        DepthByNode[RootIndex] = 0;
        RowByNode[RootIndex] = ReserveRow(0, 0);

        int32 NextBranchRow = 1;

        for (int32 QueueIndex = 0; QueueIndex < VisitQueue.Num(); ++QueueIndex)
        {
            const int32 NodeIndex = VisitQueue[QueueIndex];
            const int32 ParentRow = RowByNode[NodeIndex] != INDEX_NONE ? RowByNode[NodeIndex] : 0;
            int32 ChildSlot = 0;

            for (const int32 ChildIndex : Adjacency[NodeIndex])
            {
                if (DepthByNode[ChildIndex] != INDEX_NONE)
                {
                    ++ChildSlot;
                    continue;
                }

                const int32 ChildDepth = DepthByNode[NodeIndex] + 1;
                const int32 PreferredRow = ChildSlot == 0 ? ParentRow : NextBranchRow++;
                const int32 ChildRow = ReserveRow(ChildDepth, PreferredRow);

                DepthByNode[ChildIndex] = ChildDepth;
                RowByNode[ChildIndex] = ChildRow;
                NextBranchRow = FMath::Max(NextBranchRow, ChildRow + 1);
                VisitQueue.Add(ChildIndex);
                ++ChildSlot;
            }
        }

        int32 MaxReachableDepth = 0;
        for (int32 NodeIndex = 0; NodeIndex < DepthByNode.Num(); ++NodeIndex)
        {
            if (DepthByNode[NodeIndex] == INDEX_NONE)
            {
                continue;
            }

            MaxReachableDepth = FMath::Max(MaxReachableDepth, DepthByNode[NodeIndex]);
            if (!Layout.Nodes[NodeIndex].bIsSynthetic)
            {
                Layout.ReachableActionNodeCount++;
            }
        }

        for (int32 NodeIndex = 0; NodeIndex < DepthByNode.Num(); ++NodeIndex)
        {
            if (DepthByNode[NodeIndex] != INDEX_NONE)
            {
                continue;
            }

            DepthByNode[NodeIndex] = MaxReachableDepth + 1;
            RowByNode[NodeIndex] = ReserveRow(DepthByNode[NodeIndex], NextBranchRow++);
            if (!Layout.Nodes[NodeIndex].bIsMissingDefinition)
            {
                Layout.Nodes[NodeIndex].FillColor = FLinearColor(0.28f, 0.23f, 0.13f, 1.0f);
                Layout.Nodes[NodeIndex].Subtitle = Layout.Nodes[NodeIndex].Subtitle + TEXT("\nUnreachable from Root");
            }
        }

        int32 MaxDepth = 0;
        int32 MaxRow = 0;

        for (int32 NodeIndex = 0; NodeIndex < Layout.Nodes.Num(); ++NodeIndex)
        {
            const int32 NodeDepth = FMath::Max(DepthByNode[NodeIndex], 0);
            const int32 NodeRow = RowByNode[NodeIndex] != INDEX_NONE ? RowByNode[NodeIndex] : 0;

            MaxDepth = FMath::Max(MaxDepth, NodeDepth);
            MaxRow = FMath::Max(MaxRow, NodeRow);

            FGraphNodeVisual& Node = Layout.Nodes[NodeIndex];
            Node.Position = FVector2D(
                CanvasMargin + NodeDepth * ColumnSpacing,
                CanvasMargin + NodeRow * RowSpacing);
        }

        Layout.CanvasSize = FVector2D(
            CanvasMargin * 2.0f + NodeWidth + MaxDepth * ColumnSpacing,
            CanvasMargin * 2.0f + NodeHeight + MaxRow * RowSpacing);

        return Layout;
    }

    class FActionCombatNodeDragDropOp : public FDecoratedDragDropOp
    {
    public:
        DRAG_DROP_OPERATOR_TYPE(FActionCombatNodeDragDropOp, FDecoratedDragDropOp)

        FGameplayTag SourceActionTag;

        static TSharedRef<FActionCombatNodeDragDropOp> New(const FGameplayTag& InSourceActionTag, const FString& InLabel)
        {
            TSharedRef<FActionCombatNodeDragDropOp> Operation = MakeShared<FActionCombatNodeDragDropOp>();
            Operation->SourceActionTag = InSourceActionTag;
            Operation->DefaultHoverText = FText::Format(LOCTEXT("ConnectDragHover", "Connect from {0}"), FText::FromString(InLabel));
            Operation->Construct();
            return Operation;
        }
    };

    FGameplayTag RequestTagOrNone(const FString& TagString)
    {
        const FString Trimmed = TagString.TrimStartAndEnd();
        if (Trimmed.IsEmpty())
        {
            return FGameplayTag();
        }

        return UGameplayTagsManager::Get().RequestGameplayTag(FName(*Trimmed), false);
    }

    FGameplayTagContainer ParseTagContainerText(const FString& TagsText)
    {
        FGameplayTagContainer Result;
        TArray<FString> Parts;
        TagsText.ParseIntoArray(Parts, TEXT(","), true);

        for (FString Part : Parts)
        {
            const FGameplayTag Tag = RequestTagOrNone(Part);
            if (Tag.IsValid())
            {
                Result.AddTag(Tag);
            }
        }

        return Result;
    }

    class SActionCombatStyleGraphNode : public SCompoundWidget
    {
    public:
        SLATE_BEGIN_ARGS(SActionCombatStyleGraphNode)
        {
        }

            SLATE_ARGUMENT(FGraphNodeVisual, Node)
            SLATE_ARGUMENT(bool, IsSelected)
            SLATE_EVENT(FOnActionIndex, OnActionSelected)
            SLATE_EVENT(FOnTransitionRequested, OnTransitionRequested)
            SLATE_EVENT(FOnMontageDropped, OnMontageDropped)

        SLATE_END_ARGS()

        void Construct(const FArguments& InArgs)
        {
            Node = InArgs._Node;
            bIsSelected = InArgs._IsSelected;
            OnActionSelected = InArgs._OnActionSelected;
            OnTransitionRequested = InArgs._OnTransitionRequested;
            OnMontageDropped = InArgs._OnMontageDropped;

            ChildSlot
            [
                SNew(SBorder)
                .BorderImage(FAppStyle::GetBrush("WhiteBrush"))
                    .BorderBackgroundColor(bIsSelected ? FLinearColor(0.98f, 0.72f, 0.26f, 1.0f) : FLinearColor(0.10f, 0.11f, 0.13f, 1.0f))
                    .Padding(FMargin(3.0f))
                    [
                        SNew(SBorder)
                        .BorderImage(FAppStyle::GetBrush("WhiteBrush"))
                        .BorderBackgroundColor(Node.FillColor)
                    .Padding(FMargin(14.0f, 12.0f))
                    [
                        SNew(SVerticalBox)
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        [
                            SNew(STextBlock)
                            .Text(FText::FromString(Node.Title))
                            .Font(FAppStyle::GetFontStyle("NormalFontBold"))
                            .Justification(ETextJustify::Center)
                            .AutoWrapText(true)
                        ]
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(FMargin(0.0f, 8.0f, 0.0f, 0.0f))
                        [
                            SNew(STextBlock)
                            .Text(FText::FromString(Node.Subtitle))
                            .Font(FAppStyle::GetFontStyle("SmallFont"))
                            .ColorAndOpacity(FLinearColor(0.90f, 0.90f, 0.90f, 1.0f))
                            .Justification(ETextJustify::Center)
                            .AutoWrapText(true)
                        ]
                    ]
                ]
            ];
        }

        virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
        {
            if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
            {
                if (Node.ActionIndex != INDEX_NONE && OnActionSelected.IsBound())
                {
                    OnActionSelected.Execute(Node.ActionIndex);
                }

                if (Node.bIsSynthetic || Node.ActionTag.IsValid())
                {
                    return FReply::Handled().DetectDrag(SharedThis(this), EKeys::LeftMouseButton);
                }

                return FReply::Handled();
            }

            return FReply::Unhandled();
        }

        virtual FReply OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
        {
            if (!Node.bIsSynthetic && !Node.ActionTag.IsValid())
            {
                return FReply::Unhandled();
            }

            return FReply::Handled().BeginDragDrop(FActionCombatNodeDragDropOp::New(Node.ActionTag, Node.Title));
        }

        virtual FReply OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override
        {
            if (TSharedPtr<FActionCombatNodeDragDropOp> NodeOp = DragDropEvent.GetOperationAs<FActionCombatNodeDragDropOp>())
            {
                if (Node.ActionTag.IsValid() && OnTransitionRequested.IsBound())
                {
                    OnTransitionRequested.Execute(NodeOp->SourceActionTag, Node.ActionTag);
                    return FReply::Handled();
                }
            }

            if (Node.ActionIndex != INDEX_NONE)
            {
                if (TSharedPtr<FAssetDragDropOp> AssetOp = DragDropEvent.GetOperationAs<FAssetDragDropOp>())
                {
                    for (const FAssetData& AssetData : AssetOp->GetAssets())
                    {
                        if (UAnimMontage* Montage = Cast<UAnimMontage>(AssetData.GetAsset()))
                        {
                            if (OnMontageDropped.IsBound())
                            {
                                OnMontageDropped.Execute(Node.ActionIndex, Montage);
                            }
                            return FReply::Handled();
                        }
                    }
                }
            }

            return FReply::Unhandled();
        }

    private:
        FGraphNodeVisual Node;
        bool bIsSelected = false;
        FOnActionIndex OnActionSelected;
        FOnTransitionRequested OnTransitionRequested;
        FOnMontageDropped OnMontageDropped;
    };

    class SActionCombatStyleGraphLines : public SLeafWidget
    {
    public:
        SLATE_BEGIN_ARGS(SActionCombatStyleGraphLines)
        {
        }

            SLATE_ARGUMENT(FGraphLayout, GraphLayout)
            SLATE_ARGUMENT(UActionCombatStyleData*, StyleData)
            SLATE_ARGUMENT(int32, SelectedTransitionIndex)
            SLATE_EVENT(FSimpleDelegate, OnGraphEdited)
            SLATE_EVENT(FOnTransitionIndex, OnTransitionSelected)

        SLATE_END_ARGS()

        void Construct(const FArguments& InArgs)
        {
            GraphLayout = InArgs._GraphLayout;
            StyleData = InArgs._StyleData;
            SelectedTransitionIndex = InArgs._SelectedTransitionIndex;
            OnGraphEdited = InArgs._OnGraphEdited;
            OnTransitionSelected = InArgs._OnTransitionSelected;
        }

        virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
        {
            if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
            {
                const FVector2D LocalPosition = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
                for (const FGraphEdgeVisual& Edge : GraphLayout.Edges)
                {
                    if (GetEdgeLabelRect(Edge).ContainsPoint(LocalPosition))
                    {
                        if (OnTransitionSelected.IsBound())
                        {
                            OnTransitionSelected.Execute(Edge.TransitionIndex);
                        }
                        return FReply::Handled();
                    }
                }
            }

            if (MouseEvent.GetEffectingButton().GetFName() != TEXT("RightMouseButton"))
            {
                return FReply::Unhandled();
            }

            FMenuBuilder MenuBuilder(true, nullptr);
            MenuBuilder.AddMenuEntry(
                LOCTEXT("AddActionNode", "Add Action Node"),
                LOCTEXT("AddActionNodeTooltip", "Append a new action entry. Set its ActionTag and Montage in Details."),
                FSlateIcon(),
                FUIAction(FExecuteAction::CreateSP(this, &SActionCombatStyleGraphLines::AddActionNode)));

            FSlateApplication::Get().PushMenu(
                AsShared(),
                FWidgetPath(),
                MenuBuilder.MakeWidget(),
                MouseEvent.GetScreenSpacePosition(),
                FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu));

            return FReply::Handled();
        }

        virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
            FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
        {
            const FSlateFontInfo LabelFont = FAppStyle::GetFontStyle("SmallFont");

            for (float X = 0.0f; X <= GraphLayout.CanvasSize.X; X += 32.0f)
            {
                const TArray<FVector2D> GridPoints = { FVector2D(X, 0.0f), FVector2D(X, GraphLayout.CanvasSize.Y) };
                FSlateDrawElement::MakeLines(
                    OutDrawElements,
                    LayerId,
                    AllottedGeometry.ToPaintGeometry(),
                    GridPoints,
                    ESlateDrawEffect::None,
                    FLinearColor(0.09f, 0.10f, 0.11f, 0.35f),
                    true,
                    1.0f);
            }

            for (float Y = 0.0f; Y <= GraphLayout.CanvasSize.Y; Y += 32.0f)
            {
                const TArray<FVector2D> GridPoints = { FVector2D(0.0f, Y), FVector2D(GraphLayout.CanvasSize.X, Y) };
                FSlateDrawElement::MakeLines(
                    OutDrawElements,
                    LayerId,
                    AllottedGeometry.ToPaintGeometry(),
                    GridPoints,
                    ESlateDrawEffect::None,
                    FLinearColor(0.09f, 0.10f, 0.11f, 0.35f),
                    true,
                    1.0f);
            }

            for (const FGraphEdgeVisual& Edge : GraphLayout.Edges)
            {
                if (!GraphLayout.Nodes.IsValidIndex(Edge.FromNodeIndex) || !GraphLayout.Nodes.IsValidIndex(Edge.ToNodeIndex))
                {
                    continue;
                }

                const FLinearColor DrawColor = (Edge.TransitionIndex == SelectedTransitionIndex)
                    ? FLinearColor(1.0f, 0.77f, 0.25f, 1.0f)
                    : Edge.Color;
                TArray<FVector2D> EdgePoints;
                FVector2D ArrowDirection = FVector2D(1.0f, 0.0f);
                FVector2D EndPoint;
                const FVector2D LabelPosition = BuildEdgeGeometry(Edge, EdgePoints, EndPoint, ArrowDirection);

                FSlateDrawElement::MakeLines(
                    OutDrawElements,
                    LayerId + 1,
                    AllottedGeometry.ToPaintGeometry(),
                    EdgePoints,
                    ESlateDrawEffect::None,
                    DrawColor,
                    true,
                    Edge.TransitionIndex == SelectedTransitionIndex ? LineThickness + 1.0f : LineThickness);

                const bool bForwardEdgeLine = EdgePoints.Num() == 2 && FMath::Abs(EdgePoints[0].Y - EdgePoints[1].Y) <= KINDA_SMALL_NUMBER;
                const FVector2D ArrowPoint = bForwardEdgeLine
                    ? (EdgePoints[0] + EdgePoints[1]) * 0.5f
                    : EndPoint;
                const FVector2D Direction = ArrowDirection.GetSafeNormal();
                const FVector2D ArrowBack = -Direction * ArrowLength;
                const FVector2D ArrowSide = FVector2D(-Direction.Y, Direction.X) * ArrowHalfWidth;
                const TArray<FVector2D> ArrowHeadA =
                {
                    ArrowPoint,
                    ArrowPoint + ArrowBack + ArrowSide
                };

                const TArray<FVector2D> ArrowHeadB =
                {
                    ArrowPoint,
                    ArrowPoint + ArrowBack - ArrowSide
                };

                FSlateDrawElement::MakeLines(
                    OutDrawElements,
                    LayerId + 2,
                    AllottedGeometry.ToPaintGeometry(),
                    ArrowHeadA,
                    ESlateDrawEffect::None,
                    DrawColor,
                    true,
                    LineThickness);

                FSlateDrawElement::MakeLines(
                    OutDrawElements,
                    LayerId + 2,
                    AllottedGeometry.ToPaintGeometry(),
                    ArrowHeadB,
                    ESlateDrawEffect::None,
                    DrawColor,
                    true,
                    LineThickness);

                FSlateDrawElement::MakeText(
                    OutDrawElements,
                    LayerId + 3,
                    AllottedGeometry.ToPaintGeometry(
                        FVector2f(EdgeLabelSize.X, EdgeLabelSize.Y),
                        FSlateLayoutTransform(FVector2f(LabelPosition.X, LabelPosition.Y))),
                    Edge.Label,
                    LabelFont,
                    ESlateDrawEffect::None,
                    FLinearColor(0.92f, 0.92f, 0.92f, 1.0f));
            }

            return LayerId + 4;
        }

        virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override
        {
            return GraphLayout.CanvasSize;
        }

    private:
        FVector2D BuildEdgeGeometry(const FGraphEdgeVisual& Edge, TArray<FVector2D>& OutEdgePoints, FVector2D& OutEndPoint, FVector2D& OutArrowDirection) const
        {
            if (!GraphLayout.Nodes.IsValidIndex(Edge.FromNodeIndex) || !GraphLayout.Nodes.IsValidIndex(Edge.ToNodeIndex))
            {
                OutEndPoint = FVector2D::ZeroVector;
                OutArrowDirection = FVector2D(1.0f, 0.0f);
                return FVector2D::ZeroVector;
            }

            const FGraphNodeVisual& FromNode = GraphLayout.Nodes[Edge.FromNodeIndex];
            const FGraphNodeVisual& ToNode = GraphLayout.Nodes[Edge.ToNodeIndex];

            const bool bForwardEdge = ToNode.Position.X >= FromNode.Position.X + FromNode.Size.X;
            const float FromCenterY = FromNode.Position.Y + FromNode.Size.Y * 0.5f;
            const float ToCenterY = ToNode.Position.Y + ToNode.Size.Y * 0.5f;
            const FVector2D StartPoint = bForwardEdge
                ? FVector2D(FromNode.Position.X + FromNode.Size.X + EdgeAnchorGap, FromCenterY)
                : FVector2D(FromNode.Position.X + FromNode.Size.X * 0.5f, FromNode.Position.Y + FromNode.Size.Y + EdgeAnchorGap);
            OutEndPoint = bForwardEdge
                ? FVector2D(ToNode.Position.X - EdgeAnchorGap, ToCenterY)
                : FVector2D(ToNode.Position.X + ToNode.Size.X * 0.5f, ToNode.Position.Y - EdgeAnchorGap);

            if (bForwardEdge)
            {
                OutEdgePoints =
                {
                    StartPoint,
                    OutEndPoint
                };
                OutArrowDirection = (OutEndPoint - StartPoint).GetSafeNormal();
                return FVector2D(
                    (StartPoint.X + OutEndPoint.X) * 0.5f - EdgeLabelSize.X * 0.5f,
                    (StartPoint.Y + OutEndPoint.Y) * 0.5f + 18.0f);
            }

            const float RouteY = FMath::Max(StartPoint.Y, OutEndPoint.Y) + 56.0f;
            OutEdgePoints =
            {
                StartPoint,
                FVector2D(StartPoint.X, RouteY),
                FVector2D(OutEndPoint.X, RouteY),
                OutEndPoint
            };
            OutArrowDirection = FVector2D(0.0f, -1.0f);
            return FVector2D(
                (StartPoint.X + OutEndPoint.X) * 0.5f - EdgeLabelSize.X * 0.5f,
                RouteY + 8.0f);
        }

        FSlateRect GetEdgeLabelRect(const FGraphEdgeVisual& Edge) const
        {
            TArray<FVector2D> EdgePoints;
            FVector2D EndPoint;
            FVector2D ArrowDirection;
            const FVector2D LabelPosition = BuildEdgeGeometry(Edge, EdgePoints, EndPoint, ArrowDirection);
            return FSlateRect(
                LabelPosition.X,
                LabelPosition.Y,
                LabelPosition.X + EdgeLabelSize.X,
                LabelPosition.Y + EdgeLabelSize.Y);
        }

        void AddActionNode()
        {
            UActionCombatStyleData* Style = StyleData.Get();
            TArray<FActionCombatActionDefinition>* Actions = GetMutableActions(Style);
            if (!Style || !Actions)
            {
                return;
            }

            const FScopedTransaction Transaction(LOCTEXT("AddActionNodeTransaction", "Add Action Combat Action Node"));
            Style->Modify();

            FActionCombatActionDefinition NewAction;
            if (!Actions->IsEmpty())
            {
                NewAction = Actions->Last();
                NewAction.ActionTag = FGameplayTag();
                NewAction.Montage = nullptr;
            }

            Actions->Add(NewAction);
            Style->MarkPackageDirty();
            Style->PostEditChange();

            if (OnGraphEdited.IsBound())
            {
                OnGraphEdited.Execute();
            }
        }

        FGraphLayout GraphLayout;
        TWeakObjectPtr<UActionCombatStyleData> StyleData;
        int32 SelectedTransitionIndex = INDEX_NONE;
        FSimpleDelegate OnGraphEdited;
        FOnTransitionIndex OnTransitionSelected;
    };

    TSharedRef<SWidget> BuildGraphWidget(
        const FGraphLayout& GraphLayout,
        UActionCombatStyleData* StyleData,
        int32 SelectedActionIndex,
        int32 SelectedTransitionIndex,
        FSimpleDelegate OnGraphEdited,
        FOnActionIndex OnActionSelected,
        FOnTransitionIndex OnTransitionSelected,
        FOnTransitionRequested OnTransitionRequested,
        FOnMontageDropped OnMontageDropped)
    {
        TSharedRef<SConstraintCanvas> NodeCanvas = SNew(SConstraintCanvas);

        for (const FGraphNodeVisual& Node : GraphLayout.Nodes)
        {
            NodeCanvas->AddSlot()
                .Offset(FMargin(Node.Position.X, Node.Position.Y, Node.Size.X, Node.Size.Y))
                [
                    SNew(SActionCombatStyleGraphNode)
                    .Node(Node)
                    .IsSelected(Node.ActionIndex != INDEX_NONE && Node.ActionIndex == SelectedActionIndex)
                    .OnActionSelected(OnActionSelected)
                    .OnTransitionRequested(OnTransitionRequested)
                    .OnMontageDropped(OnMontageDropped)
                ];
        }

        return SNew(SScrollBox)
            .Orientation(Orient_Horizontal)
            + SScrollBox::Slot()
            [
                SNew(SScrollBox)
                .Orientation(Orient_Vertical)
                + SScrollBox::Slot()
                [
                    SNew(SBox)
                    .WidthOverride(GraphLayout.CanvasSize.X)
                    .HeightOverride(GraphLayout.CanvasSize.Y)
                    [
                        SNew(SOverlay)
                        + SOverlay::Slot()
                        [
                            NodeCanvas
                        ]
                        + SOverlay::Slot()
                        [
                            SNew(SActionCombatStyleGraphLines)
                            .GraphLayout(GraphLayout)
                            .StyleData(StyleData)
                            .SelectedTransitionIndex(SelectedTransitionIndex)
                            .OnGraphEdited(OnGraphEdited)
                            .OnTransitionSelected(OnTransitionSelected)
                        ]
                    ]
                ]
            ];
    }
}

void SActionCombatStyleGraphView::Construct(const FArguments& InArgs)
{
    StyleData = InArgs._StyleData;
    OnStyleDataEdited = InArgs._OnStyleDataEdited;

    ChildSlot
    [
        SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(8.0f)
        [
            SNew(SBorder)
            .Padding(10.0f)
            .BorderImage(FAppStyle::GetBrush("Brushes.Panel"))
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    SAssignNew(SummaryTextBlock, STextBlock)
                    .AutoWrapText(true)
                    .Font(FAppStyle::GetFontStyle("NormalFont"))
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(FMargin(0.0f, 6.0f, 0.0f, 0.0f))
                [
                    SNew(STextBlock)
                    .Text(LOCTEXT("InteractionHint", "Right-click empty graph space to add an action. Drag an action output pin to another action input pin to create a transition node. Drop an AnimMontage asset onto an action node to assign it. Click an action or transition node to edit common values below."))
                    .Font(FAppStyle::GetFontStyle("SmallFont"))
                    .ColorAndOpacity(FLinearColor(0.82f, 0.84f, 0.88f, 1.0f))
                    .AutoWrapText(true)
                ]
            ]
        ]
        + SVerticalBox::Slot()
        .FillHeight(1.0f)
        .Padding(FMargin(8.0f, 0.0f, 8.0f, 8.0f))
        [
            SAssignNew(GraphHost, SBox)
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(FMargin(8.0f, 0.0f, 8.0f, 8.0f))
        [
            SNew(SBox)
            .HeightOverride(160.0f)
            [
                SAssignNew(SelectionHost, SBox)
            ]
        ]
    ];

    RefreshGraph();
    RefreshSelectionEditor();
}

SActionCombatStyleGraphView::~SActionCombatStyleGraphView()
{
    if (EditorGraph)
    {
        EditorGraph->OnStyleDataEdited.Unbind();
        if (EditorGraph->IsRooted())
        {
            EditorGraph->RemoveFromRoot();
        }
        EditorGraph = nullptr;
    }
}

void SActionCombatStyleGraphView::RefreshGraph()
{
    CaptureGraphNodePositions();
    UActionCombatStyleData* Style = StyleData.Get();

    if (!GraphHost.IsValid())
    {
        return;
    }

    if (!Style)
    {
        GraphHost->SetContent(
            SNew(SBorder)
            .Padding(16.0f)
            [
                SNew(STextBlock)
                .Text(LOCTEXT("NoStyleData", "No style data is loaded."))
            ]);
        return;
    }

    if (!EditorGraph)
    {
        EditorGraph = NewObject<UActionCombatStyleGraph>(GetTransientPackage(), UActionCombatStyleGraph::StaticClass());
        EditorGraph->AddToRoot();
    }

    EditorGraph->Initialize(Style);
    EditorGraph->OnStyleDataEdited = FSimpleDelegate::CreateSP(this, &SActionCombatStyleGraphView::HandleGraphDataEdited);
    EditorGraph->RebuildFromStyleData(SavedNodePositions);
    UpdateSummaryText();

    SGraphEditor::FGraphEditorEvents GraphEvents;
    GraphEvents.OnSelectionChanged = SGraphEditor::FOnSelectionChanged::CreateSP(this, &SActionCombatStyleGraphView::HandleGraphSelectionChanged);

    GraphHost->SetContent(
        SAssignNew(GraphEditor, SGraphEditor)
        .GraphToEdit(EditorGraph)
        .GraphEvents(GraphEvents)
        .IsEditable(true)
        .ShowGraphStateOverlay(false));
}

void SActionCombatStyleGraphView::HandleGraphEdited()
{
    if (OnStyleDataEdited.IsBound())
    {
        OnStyleDataEdited.Execute();
    }

    RefreshGraph();
    RefreshSelectionEditor();
}

void SActionCombatStyleGraphView::HandleGraphDataEdited()
{
    if (OnStyleDataEdited.IsBound())
    {
        OnStyleDataEdited.Execute();
    }

    UpdateSummaryText();
    RefreshSelectionEditor();
}

void SActionCombatStyleGraphView::HandleGraphSelectionChanged(const FGraphPanelSelectionSet& NewSelection)
{
    SelectedActionIndex = INDEX_NONE;
    SelectedTransitionIndex = INDEX_NONE;

    for (UObject* SelectedObject : NewSelection)
    {
        if (const UActionCombatStyleTransitionGraphNode* TransitionNode = Cast<UActionCombatStyleTransitionGraphNode>(SelectedObject))
        {
            SelectedTransitionIndex = TransitionNode->TransitionIndex;
            break;
        }

        if (const UActionCombatStyleActionGraphNode* ActionNode = Cast<UActionCombatStyleActionGraphNode>(SelectedObject))
        {
            if (ActionNode->ActionIndex != INDEX_NONE)
            {
                SelectedActionIndex = ActionNode->ActionIndex;
                break;
            }
        }
    }

    RefreshSelectionEditor();
}

void SActionCombatStyleGraphView::CaptureGraphNodePositions()
{
    if (EditorGraph)
    {
        EditorGraph->CaptureNodePositions(SavedNodePositions);
    }
}

void SActionCombatStyleGraphView::UpdateSummaryText()
{
    if (!SummaryTextBlock.IsValid() || !EditorGraph)
    {
        return;
    }

    SummaryTextBlock->SetText(FText::Format(
        LOCTEXT("SummaryFormat", "Actions: {0} | Transitions: {1} | Reachable From Root: {2} | Missing Definitions: {3}"),
        EditorGraph->GetActionCount(),
        EditorGraph->GetTransitionCount(),
        EditorGraph->GetReachableActionNodeCount(),
        EditorGraph->GetMissingDefinitionCount()));
}

void SActionCombatStyleGraphView::SelectAction(int32 ActionIndex)
{
    SelectedActionIndex = ActionIndex;
    SelectedTransitionIndex = INDEX_NONE;
    RefreshSelectionEditor();
}

void SActionCombatStyleGraphView::SelectTransition(int32 TransitionIndex)
{
    SelectedActionIndex = INDEX_NONE;
    SelectedTransitionIndex = TransitionIndex;
    RefreshSelectionEditor();
}

void SActionCombatStyleGraphView::AddTransition(FGameplayTag FromActionTag, FGameplayTag ToActionTag)
{
    UActionCombatStyleData* Style = StyleData.Get();
    TArray<FActionCombatTransitionDefinition>* Transitions = ActionCombatStyleGraphView::GetMutableTransitions(Style);
    if (!Style || !Transitions || !ToActionTag.IsValid())
    {
        return;
    }

    FActionCombatTransitionDefinition NewTransition;
    NewTransition.FromActionTag = FromActionTag;
    NewTransition.ToActionTag = ToActionTag;
    NewTransition.CommandTag = ActionCombatStyleGraphView::RequestTagOrNone(TEXT("Combat.Command.Light"));

    const FScopedTransaction Transaction(LOCTEXT("AddTransitionTransaction", "Add Action Combat Transition"));
    Style->Modify();
    const int32 NewIndex = Transitions->Add(NewTransition);
    Style->MarkPackageDirty();
    Style->PostEditChange();

    SelectTransition(NewIndex);
    HandleGraphEdited();
}

void SActionCombatStyleGraphView::AssignMontage(int32 ActionIndex, UAnimMontage* Montage)
{
    ModifyAction(
        ActionIndex,
        [Montage](FActionCombatActionDefinition& Action)
        {
            Action.Montage = Montage;
        },
        LOCTEXT("AssignMontageTransaction", "Assign Action Combat Montage"));
}

void SActionCombatStyleGraphView::ModifyAction(int32 ActionIndex, TFunctionRef<void(FActionCombatActionDefinition&)> Edit, const FText& TransactionText)
{
    UActionCombatStyleData* Style = StyleData.Get();
    TArray<FActionCombatActionDefinition>* Actions = ActionCombatStyleGraphView::GetMutableActions(Style);
    if (!Style || !Actions || !Actions->IsValidIndex(ActionIndex))
    {
        return;
    }

    const FScopedTransaction Transaction(TransactionText);
    Style->Modify();
    Edit((*Actions)[ActionIndex]);
    Style->MarkPackageDirty();
    Style->PostEditChange();
    HandleGraphEdited();
}

void SActionCombatStyleGraphView::ModifyTransition(int32 TransitionIndex, TFunctionRef<void(FActionCombatTransitionDefinition&)> Edit, const FText& TransactionText)
{
    UActionCombatStyleData* Style = StyleData.Get();
    TArray<FActionCombatTransitionDefinition>* Transitions = ActionCombatStyleGraphView::GetMutableTransitions(Style);
    if (!Style || !Transitions || !Transitions->IsValidIndex(TransitionIndex))
    {
        return;
    }

    const FScopedTransaction Transaction(TransactionText);
    Style->Modify();
    Edit((*Transitions)[TransitionIndex]);
    Style->MarkPackageDirty();
    Style->PostEditChange();
    HandleGraphEdited();
}

void SActionCombatStyleGraphView::RefreshSelectionEditor()
{
    if (SelectionHost.IsValid())
    {
        SelectionHost->SetContent(BuildSelectionEditor());
    }
}

TSharedRef<SWidget> SActionCombatStyleGraphView::BuildSelectionEditor()
{
    UActionCombatStyleData* Style = StyleData.Get();
    TArray<FActionCombatActionDefinition>* Actions = ActionCombatStyleGraphView::GetMutableActions(Style);
    TArray<FActionCombatTransitionDefinition>* Transitions = ActionCombatStyleGraphView::GetMutableTransitions(Style);

    auto BuildRow = [](const FText& Label, TSharedRef<SWidget> ValueWidget)
    {
        return SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .Padding(FMargin(0.0f, 0.0f, 10.0f, 4.0f))
            [
                SNew(STextBlock)
                .Text(Label)
                .MinDesiredWidth(150.0f)
                .Font(FAppStyle::GetFontStyle("SmallFont"))
            ]
            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            .Padding(FMargin(0.0f, 0.0f, 0.0f, 4.0f))
            [
                ValueWidget
            ];
    };

    if (Actions && Actions->IsValidIndex(SelectedActionIndex))
    {
        const int32 ActionIndex = SelectedActionIndex;
        const FActionCombatActionDefinition& Action = (*Actions)[ActionIndex];

        auto FloatValue = [this, ActionIndex](float FActionCombatActionDefinition::* Field) -> TOptional<float>
        {
            if (TArray<FActionCombatActionDefinition>* CurrentActions = ActionCombatStyleGraphView::GetMutableActions(StyleData.Get()))
            {
                if (CurrentActions->IsValidIndex(ActionIndex))
                {
                    return (*CurrentActions)[ActionIndex].*Field;
                }
            }
            return TOptional<float>();
        };

        auto FloatEditor = [this, ActionIndex, FloatValue](float FActionCombatActionDefinition::* Field, const FText& TransactionText)
        {
            return SNew(SNumericEntryBox<float>)
                .Value_Lambda([FloatValue, Field]() { return FloatValue(Field); })
                .OnValueCommitted_Lambda([this, ActionIndex, Field, TransactionText](float NewValue, ETextCommit::Type)
                {
                    ModifyAction(ActionIndex, [Field, NewValue](FActionCombatActionDefinition& MutableAction)
                    {
                        MutableAction.*Field = NewValue;
                    }, TransactionText);
                });
        };

        return SNew(SBorder)
            .BorderImage(FAppStyle::GetBrush("Brushes.Panel"))
            .Padding(10.0f)
            [
                SNew(SScrollBox)
                .Orientation(Orient_Vertical)
                + SScrollBox::Slot()
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot().AutoHeight()
                    [
                        SNew(STextBlock)
                        .Text(FText::Format(LOCTEXT("ActionSelectionTitle", "Action Node [{0}]"), ActionIndex))
                        .Font(FAppStyle::GetFontStyle("NormalFontBold"))
                    ]
                    + SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.0f, 8.0f, 0.0f, 0.0f))
                    [
                        BuildRow(LOCTEXT("ActionTagLabel", "Action Tag"),
                            SNew(SEditableTextBox)
                            .Text(FText::FromString(Action.ActionTag.ToString()))
                            .OnTextCommitted_Lambda([this, ActionIndex](const FText& NewText, ETextCommit::Type)
                            {
                                const FGameplayTag NewTag = ActionCombatStyleGraphView::RequestTagOrNone(NewText.ToString());
                                ModifyAction(ActionIndex, [NewTag](FActionCombatActionDefinition& MutableAction)
                                {
                                    MutableAction.ActionTag = NewTag;
                                }, LOCTEXT("EditActionTagTransaction", "Edit Action Combat Action Tag"));
                            }))
                    ]
                    + SVerticalBox::Slot().AutoHeight()
                    [
                        BuildRow(LOCTEXT("MontageLabel", "Montage"),
                            SNew(SObjectPropertyEntryBox)
                            .AllowedClass(UAnimMontage::StaticClass())
                            .ObjectPath(Action.Montage ? Action.Montage->GetPathName() : FString())
                            .OnObjectChanged_Lambda([this, ActionIndex](const FAssetData& AssetData)
                            {
                                AssignMontage(ActionIndex, Cast<UAnimMontage>(AssetData.GetAsset()));
                            }))
                    ]
                    + SVerticalBox::Slot().AutoHeight()[BuildRow(LOCTEXT("FallbackDurationLabel", "Fallback Duration"), FloatEditor(&FActionCombatActionDefinition::FallbackDurationSeconds, LOCTEXT("EditFallbackDurationTransaction", "Edit Fallback Duration")))]
                    + SVerticalBox::Slot().AutoHeight()[BuildRow(LOCTEXT("BasePlayRateLabel", "Base Play Rate"), FloatEditor(&FActionCombatActionDefinition::BasePlayRate, LOCTEXT("EditBasePlayRateTransaction", "Edit Base Play Rate")))]
                    + SVerticalBox::Slot().AutoHeight()[BuildRow(LOCTEXT("QueueStartLabel", "Queue Start"), FloatEditor(&FActionCombatActionDefinition::QueueWindowStartsAtNormalizedTime, LOCTEXT("EditQueueStartTransaction", "Edit Queue Start")))]
                    + SVerticalBox::Slot().AutoHeight()[BuildRow(LOCTEXT("QueueCloseLabel", "Queue Close"), FloatEditor(&FActionCombatActionDefinition::QueueWindowClosesAtNormalizedTime, LOCTEXT("EditQueueCloseTransaction", "Edit Queue Close")))]
                    + SVerticalBox::Slot().AutoHeight()[BuildRow(LOCTEXT("CommitLabel", "Commit Time"), FloatEditor(&FActionCombatActionDefinition::ChainCommitAtNormalizedTime, LOCTEXT("EditCommitTransaction", "Edit Commit Time")))]
                    + SVerticalBox::Slot().AutoHeight()[BuildRow(LOCTEXT("MotionValueLabel", "Motion Value"), FloatEditor(&FActionCombatActionDefinition::MotionValue, LOCTEXT("EditMotionValueTransaction", "Edit Motion Value")))]
                    + SVerticalBox::Slot().AutoHeight()[BuildRow(LOCTEXT("PoiseDamageLabel", "Poise Damage"), FloatEditor(&FActionCombatActionDefinition::PoiseDamage, LOCTEXT("EditPoiseDamageTransaction", "Edit Poise Damage")))]
                    + SVerticalBox::Slot().AutoHeight()[BuildRow(LOCTEXT("BuildupLabel", "Buildup Multiplier"), FloatEditor(&FActionCombatActionDefinition::BuildupMultiplier, LOCTEXT("EditBuildupTransaction", "Edit Buildup Multiplier")))]
                    + SVerticalBox::Slot().AutoHeight()
                    [
                        BuildRow(LOCTEXT("TraceSourceLabel", "Trace Source Id"),
                            SNew(SEditableTextBox)
                            .Text(FText::FromName(Action.TraceSourceId))
                            .OnTextCommitted_Lambda([this, ActionIndex](const FText& NewText, ETextCommit::Type)
                            {
                                const FName NewName(*NewText.ToString());
                                ModifyAction(ActionIndex, [NewName](FActionCombatActionDefinition& MutableAction)
                                {
                                    MutableAction.TraceSourceId = NewName;
                                }, LOCTEXT("EditTraceSourceTransaction", "Edit Trace Source Id"));
                            }))
                    ]
                    + SVerticalBox::Slot().AutoHeight()
                    [
                        BuildRow(LOCTEXT("HitWindowLabel", "Hit Window Name"),
                            SNew(SEditableTextBox)
                            .Text(FText::FromName(Action.HitWindowName))
                            .OnTextCommitted_Lambda([this, ActionIndex](const FText& NewText, ETextCommit::Type)
                            {
                                const FName NewName(*NewText.ToString());
                                ModifyAction(ActionIndex, [NewName](FActionCombatActionDefinition& MutableAction)
                                {
                                    MutableAction.HitWindowName = NewName;
                                }, LOCTEXT("EditHitWindowTransaction", "Edit Hit Window Name"));
                            }))
                    ]
                ]
            ];
    }

    if (Transitions && Transitions->IsValidIndex(SelectedTransitionIndex))
    {
        const int32 TransitionIndex = SelectedTransitionIndex;
        const FActionCombatTransitionDefinition& Transition = (*Transitions)[TransitionIndex];

        auto TagEditor = [this, TransitionIndex, BuildRow](const FText& Label, const FGameplayTag& CurrentTag, FGameplayTag FActionCombatTransitionDefinition::* Field, const FText& TransactionText)
        {
            return BuildRow(Label,
                SNew(SEditableTextBox)
                .Text(FText::FromString(CurrentTag.ToString()))
                .OnTextCommitted_Lambda([this, TransitionIndex, Field, TransactionText](const FText& NewText, ETextCommit::Type)
                {
                    const FGameplayTag NewTag = ActionCombatStyleGraphView::RequestTagOrNone(NewText.ToString());
                    ModifyTransition(TransitionIndex, [Field, NewTag](FActionCombatTransitionDefinition& MutableTransition)
                    {
                        MutableTransition.*Field = NewTag;
                    }, TransactionText);
                }));
        };

        auto ContainerEditor = [this, TransitionIndex, BuildRow](const FText& Label, const FGameplayTagContainer& CurrentTags, FGameplayTagContainer FActionCombatTransitionDefinition::* Field, const FText& TransactionText)
        {
            return BuildRow(Label,
                SNew(SEditableTextBox)
                .Text(FText::FromString(ActionCombatStyleGraphView::FormatTagContainerList(CurrentTags)))
                .OnTextCommitted_Lambda([this, TransitionIndex, Field, TransactionText](const FText& NewText, ETextCommit::Type)
                {
                    const FGameplayTagContainer NewTags = ActionCombatStyleGraphView::ParseTagContainerText(NewText.ToString());
                    ModifyTransition(TransitionIndex, [Field, NewTags](FActionCombatTransitionDefinition& MutableTransition)
                    {
                        MutableTransition.*Field = NewTags;
                    }, TransactionText);
                }));
        };

        return SNew(SBorder)
            .BorderImage(FAppStyle::GetBrush("Brushes.Panel"))
            .Padding(10.0f)
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight()
                [
                    SNew(STextBlock)
                    .Text(FText::Format(LOCTEXT("TransitionSelectionTitle", "Transition Edge [{0}]"), TransitionIndex))
                    .Font(FAppStyle::GetFontStyle("NormalFontBold"))
                ]
                + SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.0f, 8.0f, 0.0f, 0.0f))[TagEditor(LOCTEXT("FromActionTagLabel", "From Action Tag"), Transition.FromActionTag, &FActionCombatTransitionDefinition::FromActionTag, LOCTEXT("EditFromTagTransaction", "Edit From Action Tag"))]
                + SVerticalBox::Slot().AutoHeight()[TagEditor(LOCTEXT("CommandTagLabel", "Command Tag"), Transition.CommandTag, &FActionCombatTransitionDefinition::CommandTag, LOCTEXT("EditCommandTagTransaction", "Edit Command Tag"))]
                + SVerticalBox::Slot().AutoHeight()[TagEditor(LOCTEXT("ToActionTagLabel", "To Action Tag"), Transition.ToActionTag, &FActionCombatTransitionDefinition::ToActionTag, LOCTEXT("EditToTagTransaction", "Edit To Action Tag"))]
                + SVerticalBox::Slot().AutoHeight()
                [
                    BuildRow(LOCTEXT("FocusActiveLabel", "Requires Focus Active"),
                        SNew(SCheckBox)
                        .IsChecked(Transition.bRequiresFocusActive ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
                        .OnCheckStateChanged_Lambda([this, TransitionIndex](ECheckBoxState NewState)
                        {
                            ModifyTransition(TransitionIndex, [NewState](FActionCombatTransitionDefinition& MutableTransition)
                            {
                                MutableTransition.bRequiresFocusActive = NewState == ECheckBoxState::Checked;
                            }, LOCTEXT("EditFocusActiveTransaction", "Edit Focus Active Requirement"));
                        }))
                ]
                + SVerticalBox::Slot().AutoHeight()
                [
                    BuildRow(LOCTEXT("FocusInactiveLabel", "Requires Focus Inactive"),
                        SNew(SCheckBox)
                        .IsChecked(Transition.bRequiresFocusInactive ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
                        .OnCheckStateChanged_Lambda([this, TransitionIndex](ECheckBoxState NewState)
                        {
                            ModifyTransition(TransitionIndex, [NewState](FActionCombatTransitionDefinition& MutableTransition)
                            {
                                MutableTransition.bRequiresFocusInactive = NewState == ECheckBoxState::Checked;
                            }, LOCTEXT("EditFocusInactiveTransaction", "Edit Focus Inactive Requirement"));
                        }))
                ]
                + SVerticalBox::Slot().AutoHeight()[ContainerEditor(LOCTEXT("RequiredHeldLabel", "Required Held Tags"), Transition.RequiredHeldInputTags, &FActionCombatTransitionDefinition::RequiredHeldInputTags, LOCTEXT("EditRequiredHeldTransaction", "Edit Required Held Tags"))]
                + SVerticalBox::Slot().AutoHeight()[ContainerEditor(LOCTEXT("BlockedHeldLabel", "Blocked Held Tags"), Transition.BlockedHeldInputTags, &FActionCombatTransitionDefinition::BlockedHeldInputTags, LOCTEXT("EditBlockedHeldTransaction", "Edit Blocked Held Tags"))]
            ];
    }

    return SNew(SBorder)
        .BorderImage(FAppStyle::GetBrush("Brushes.Panel"))
        .Padding(10.0f)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("NoSelectionHint", "Select a node or click an edge label to edit common values here. Full arrays remain available in Details."))
            .Font(FAppStyle::GetFontStyle("SmallFont"))
            .ColorAndOpacity(FLinearColor(0.82f, 0.84f, 0.88f, 1.0f))
            .AutoWrapText(true)
        ];
}

#undef LOCTEXT_NAMESPACE
