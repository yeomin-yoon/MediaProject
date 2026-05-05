#include "SActionCombatStyleGraphView.h"

#include "ActionCombatStyleData.h"

#include "Framework/Application/SlateApplication.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Rendering/DrawElements.h"
#include "ScopedTransaction.h"
#include "Styling/AppStyle.h"
#include "UObject/UnrealType.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SConstraintCanvas.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "SActionCombatStyleGraphView"

namespace ActionCombatStyleGraphView
{
    constexpr float CanvasMargin = 80.0f;
    constexpr float NodeWidth = 300.0f;
    constexpr float NodeHeight = 108.0f;
    constexpr float ColumnSpacing = 620.0f;
    constexpr float RowSpacing = 156.0f;
    constexpr float LineThickness = 2.0f;
    constexpr float EdgeAnchorGap = 18.0f;
    constexpr float ArrowLength = 12.0f;
    constexpr float ArrowHalfWidth = 6.0f;
    const FVector2D EdgeLabelSize = FVector2D(260.0f, 70.0f);

    struct FGraphNodeVisual
    {
        FString Key;
        FString Title;
        FString Subtitle;
        FVector2D Position = FVector2D::ZeroVector;
        FVector2D Size = FVector2D(NodeWidth, NodeHeight);
        FLinearColor FillColor = FLinearColor(0.17f, 0.20f, 0.24f, 1.0f);
        bool bIsSynthetic = false;
        bool bIsMissingDefinition = false;
    };

    struct FGraphEdgeVisual
    {
        int32 FromNodeIndex = INDEX_NONE;
        int32 ToNodeIndex = INDEX_NONE;
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

    FString FormatActionSubtitle(const FActionCombatActionDefinition& Action)
    {
        const FString QueueWindowText = FString::Printf(
            TEXT("Queue %.2f-%.2f | Commit %.2f"),
            Action.QueueWindowStartsAtNormalizedTime,
            Action.QueueWindowClosesAtNormalizedTime,
            Action.ChainCommitAtNormalizedTime);

        if (Action.Montage)
        {
            return FString::Printf(TEXT("%s\nMontage %s"), *QueueWindowText, *Action.Montage->GetName());
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

        Parts.Add(Transition.CommandTag.IsValid() ? Transition.CommandTag.ToString() : TEXT("Invalid Command"));

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
                Node.Subtitle = TEXT("Neutral entry routes");
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
                [&Action, &ActionKey, bHasValidActionTag](FGraphNodeVisual& Node)
                {
                    Node.Key = ActionKey;
                    Node.Title = bHasValidActionTag ? ActionKey : TEXT("New Action - set ActionTag");
                    Node.Subtitle = FormatActionSubtitle(Action);
                    Node.FillColor = bHasValidActionTag
                        ? FLinearColor(0.20f, 0.21f, 0.25f, 1.0f)
                        : FLinearColor(0.31f, 0.25f, 0.12f, 1.0f);
                });
        }

        TArray<TArray<int32>> Adjacency;
        Adjacency.SetNum(Layout.Nodes.Num());

        for (const FActionCombatTransitionDefinition& Transition : Transitions)
        {
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

        TArray<int32> VisitQueue;
        VisitQueue.Add(RootIndex);
        DepthByNode[RootIndex] = 0;

        for (int32 QueueIndex = 0; QueueIndex < VisitQueue.Num(); ++QueueIndex)
        {
            const int32 NodeIndex = VisitQueue[QueueIndex];
            for (const int32 ChildIndex : Adjacency[NodeIndex])
            {
                if (DepthByNode[ChildIndex] != INDEX_NONE)
                {
                    continue;
                }

                DepthByNode[ChildIndex] = DepthByNode[NodeIndex] + 1;
                VisitQueue.Add(ChildIndex);
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
            if (!Layout.Nodes[NodeIndex].bIsMissingDefinition)
            {
                Layout.Nodes[NodeIndex].FillColor = FLinearColor(0.28f, 0.23f, 0.13f, 1.0f);
                Layout.Nodes[NodeIndex].Subtitle = Layout.Nodes[NodeIndex].Subtitle + TEXT("\nUnreachable from Root");
            }
        }

        TMap<int32, TArray<int32>> NodesByDepth;
        for (int32 NodeIndex = 0; NodeIndex < Layout.Nodes.Num(); ++NodeIndex)
        {
            NodesByDepth.FindOrAdd(DepthByNode[NodeIndex]).Add(NodeIndex);
        }

        int32 MaxNodesInColumn = 1;
        int32 MaxDepth = 0;

        for (TPair<int32, TArray<int32>>& Pair : NodesByDepth)
        {
            Pair.Value.Sort(
                [&Layout](int32 LeftIndex, int32 RightIndex)
                {
                    return Layout.Nodes[LeftIndex].Title < Layout.Nodes[RightIndex].Title;
                });

            MaxNodesInColumn = FMath::Max(MaxNodesInColumn, Pair.Value.Num());
            MaxDepth = FMath::Max(MaxDepth, Pair.Key);

            for (int32 ColumnIndex = 0; ColumnIndex < Pair.Value.Num(); ++ColumnIndex)
            {
                FGraphNodeVisual& Node = Layout.Nodes[Pair.Value[ColumnIndex]];
                Node.Position = FVector2D(
                    CanvasMargin + Pair.Key * ColumnSpacing,
                    CanvasMargin + ColumnIndex * RowSpacing);
            }
        }

        Layout.CanvasSize = FVector2D(
            CanvasMargin * 2.0f + NodeWidth + MaxDepth * ColumnSpacing,
            CanvasMargin * 2.0f + NodeHeight + (MaxNodesInColumn - 1) * RowSpacing);

        return Layout;
    }

    class SActionCombatStyleGraphLines : public SLeafWidget
    {
    public:
        SLATE_BEGIN_ARGS(SActionCombatStyleGraphLines)
        {
        }

            SLATE_ARGUMENT(FGraphLayout, GraphLayout)
            SLATE_ARGUMENT(UActionCombatStyleData*, StyleData)
            SLATE_EVENT(FSimpleDelegate, OnGraphEdited)

        SLATE_END_ARGS()

        void Construct(const FArguments& InArgs)
        {
            GraphLayout = InArgs._GraphLayout;
            StyleData = InArgs._StyleData;
            OnGraphEdited = InArgs._OnGraphEdited;
        }

        virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
        {
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

                const FGraphNodeVisual& FromNode = GraphLayout.Nodes[Edge.FromNodeIndex];
                const FGraphNodeVisual& ToNode = GraphLayout.Nodes[Edge.ToNodeIndex];

                const bool bForwardEdge = ToNode.Position.X >= FromNode.Position.X + FromNode.Size.X;
                const FVector2D StartPoint = bForwardEdge
                    ? FromNode.Position + FVector2D(FromNode.Size.X + EdgeAnchorGap, FromNode.Size.Y * 0.5f)
                    : FromNode.Position + FVector2D(FromNode.Size.X * 0.5f, FromNode.Size.Y + EdgeAnchorGap);
                const FVector2D EndPoint = bForwardEdge
                    ? ToNode.Position + FVector2D(-EdgeAnchorGap, ToNode.Size.Y * 0.5f)
                    : ToNode.Position + FVector2D(ToNode.Size.X * 0.5f, -EdgeAnchorGap);

                TArray<FVector2D> EdgePoints;
                FVector2D ArrowDirection = FVector2D(1.0f, 0.0f);
                FVector2D LabelPosition;

                if (bForwardEdge)
                {
                    const float MidX = (StartPoint.X + EndPoint.X) * 0.5f;
                    const float LabelX = StartPoint.X + FMath::Max(0.0f, EndPoint.X - StartPoint.X - EdgeLabelSize.X) * 0.5f;
                    EdgePoints =
                    {
                        StartPoint,
                        FVector2D(MidX, StartPoint.Y),
                        FVector2D(MidX, EndPoint.Y),
                        EndPoint
                    };
                    ArrowDirection = FVector2D(1.0f, 0.0f);
                    LabelPosition = FVector2D(
                        LabelX,
                        (StartPoint.Y + EndPoint.Y) * 0.5f - EdgeLabelSize.Y * 0.5f);
                }
                else
                {
                    const float RouteY = FMath::Max(StartPoint.Y, EndPoint.Y) + 56.0f;
                    EdgePoints =
                    {
                        StartPoint,
                        FVector2D(StartPoint.X, RouteY),
                        FVector2D(EndPoint.X, RouteY),
                        EndPoint
                    };
                    ArrowDirection = FVector2D(0.0f, -1.0f);
                    LabelPosition = FVector2D(
                        (StartPoint.X + EndPoint.X) * 0.5f - EdgeLabelSize.X * 0.5f,
                        RouteY + 8.0f);
                }

                FSlateDrawElement::MakeLines(
                    OutDrawElements,
                    LayerId + 1,
                    AllottedGeometry.ToPaintGeometry(),
                    EdgePoints,
                    ESlateDrawEffect::None,
                    Edge.Color,
                    true,
                    LineThickness);

                const FVector2D Direction = ArrowDirection.GetSafeNormal();
                const FVector2D ArrowBack = -Direction * ArrowLength;
                const FVector2D ArrowSide = FVector2D(-Direction.Y, Direction.X) * ArrowHalfWidth;
                const TArray<FVector2D> ArrowHeadA =
                {
                    EndPoint,
                    EndPoint + ArrowBack + ArrowSide
                };

                const TArray<FVector2D> ArrowHeadB =
                {
                    EndPoint,
                    EndPoint + ArrowBack - ArrowSide
                };

                FSlateDrawElement::MakeLines(
                    OutDrawElements,
                    LayerId + 2,
                    AllottedGeometry.ToPaintGeometry(),
                    ArrowHeadA,
                    ESlateDrawEffect::None,
                    Edge.Color,
                    true,
                    LineThickness);

                FSlateDrawElement::MakeLines(
                    OutDrawElements,
                    LayerId + 2,
                    AllottedGeometry.ToPaintGeometry(),
                    ArrowHeadB,
                    ESlateDrawEffect::None,
                    Edge.Color,
                    true,
                    LineThickness);

                FSlateDrawElement::MakeBox(
                    OutDrawElements,
                    LayerId + 3,
                    AllottedGeometry.ToPaintGeometry(
                        FVector2f(EdgeLabelSize.X, EdgeLabelSize.Y),
                        FSlateLayoutTransform(FVector2f(LabelPosition.X, LabelPosition.Y))),
                    FAppStyle::GetBrush("WhiteBrush"),
                    ESlateDrawEffect::None,
                    FLinearColor(0.07f, 0.08f, 0.09f, 0.88f));

                FSlateDrawElement::MakeText(
                    OutDrawElements,
                    LayerId + 4,
                    AllottedGeometry.ToPaintGeometry(
                        FVector2f(EdgeLabelSize.X - 12.0f, EdgeLabelSize.Y - 8.0f),
                        FSlateLayoutTransform(FVector2f(LabelPosition.X + 6.0f, LabelPosition.Y + 4.0f))),
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
        FSimpleDelegate OnGraphEdited;
    };

    TSharedRef<SWidget> BuildNodeWidget(const FGraphNodeVisual& Node)
    {
        return SNew(SBorder)
            .BorderImage(FAppStyle::GetBrush("WhiteBrush"))
            .BorderBackgroundColor(Node.FillColor)
            .Padding(FMargin(12.0f, 10.0f))
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(Node.Title))
                    .Font(FAppStyle::GetFontStyle("NormalFontBold"))
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
                    .AutoWrapText(true)
                ]
            ];
    }

    TSharedRef<SWidget> BuildGraphWidget(const FGraphLayout& GraphLayout, UActionCombatStyleData* StyleData, FSimpleDelegate OnGraphEdited)
    {
        TSharedRef<SConstraintCanvas> NodeCanvas = SNew(SConstraintCanvas);

        for (const FGraphNodeVisual& Node : GraphLayout.Nodes)
        {
            NodeCanvas->AddSlot()
                .Offset(FMargin(Node.Position.X, Node.Position.Y, Node.Size.X, Node.Size.Y))
                [
                    BuildNodeWidget(Node)
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
                            SNew(SActionCombatStyleGraphLines)
                            .GraphLayout(GraphLayout)
                            .StyleData(StyleData)
                            .OnGraphEdited(OnGraphEdited)
                        ]
                        + SOverlay::Slot()
                        [
                            NodeCanvas
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
                    .Text(LOCTEXT("ReadOnlyHint", "This graph is read-only. Edit actions and transitions in Details, then the view refreshes automatically."))
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
    ];

    RefreshGraph();
}

void SActionCombatStyleGraphView::RefreshGraph()
{
    const UActionCombatStyleData* Style = StyleData.Get();
    const ActionCombatStyleGraphView::FGraphLayout GraphLayout = ActionCombatStyleGraphView::BuildGraphLayout(Style);

    if (SummaryTextBlock.IsValid())
    {
        SummaryTextBlock->SetText(FText::Format(
            LOCTEXT("SummaryFormat", "Actions: {0} | Transitions: {1} | Reachable From Root: {2} | Missing Definitions: {3}"),
            GraphLayout.ActionCount,
            GraphLayout.TransitionCount,
            GraphLayout.ReachableActionNodeCount,
            GraphLayout.MissingDefinitionCount));
    }

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

    GraphHost->SetContent(ActionCombatStyleGraphView::BuildGraphWidget(
        GraphLayout,
        StyleData.Get(),
        FSimpleDelegate::CreateSP(this, &SActionCombatStyleGraphView::HandleGraphEdited)));
}

void SActionCombatStyleGraphView::HandleGraphEdited()
{
    if (OnStyleDataEdited.IsBound())
    {
        OnStyleDataEdited.Execute();
    }

    RefreshGraph();
}

#undef LOCTEXT_NAMESPACE
