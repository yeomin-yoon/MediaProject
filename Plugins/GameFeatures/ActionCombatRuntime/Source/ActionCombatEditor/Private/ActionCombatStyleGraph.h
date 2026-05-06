#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphSchema.h"
#include "GameplayTagContainer.h"

#include "ActionCombatStyleGraph.generated.h"

class FConnectionDrawingPolicy;
class UActionCombatStyleData;
class UAnimMontage;

UCLASS()
class UActionCombatStyleGraph : public UEdGraph
{
    GENERATED_BODY()

public:
    void Initialize(UActionCombatStyleData* InStyleData);
    void RebuildFromStyleData(const TMap<FString, FVector2D>& SavedNodePositions);
    void CaptureNodePositions(TMap<FString, FVector2D>& OutNodePositions) const;

    class UActionCombatStyleActionGraphNode* AddActionNodeAt(const FVector2D& GraphPosition);
    class UActionCombatStyleTransitionGraphNode* AddTransitionNodeBetween(class UActionCombatStyleActionGraphNode* FromNode, class UActionCombatStyleActionGraphNode* ToNode, const FVector2D& GraphPosition);
    bool AssignMontageToActionNode(class UActionCombatStyleActionGraphNode* ActionNode, UAnimMontage* Montage);

    UActionCombatStyleData* GetStyleData() const { return StyleData.Get(); }
    void NotifyStyleDataEdited();

    int32 GetActionCount() const { return ActionCount; }
    int32 GetTransitionCount() const { return TransitionCount; }
    int32 GetReachableActionNodeCount() const { return ReachableActionNodeCount; }
    int32 GetMissingDefinitionCount() const { return MissingDefinitionCount; }

    FSimpleDelegate OnStyleDataEdited;

private:
    class UActionCombatStyleActionGraphNode* CreateActionVisualNode(int32 ActionIndex, const FGameplayTag& ActionTag, bool bRootNode, bool bMissingDefinition, const FVector2D& GraphPosition);
    class UActionCombatStyleTransitionGraphNode* CreateTransitionVisualNode(int32 TransitionIndex, const FVector2D& GraphPosition);
    void RebuildPinsAndLinks(const TMap<FString, class UActionCombatStyleActionGraphNode*>& ActionNodesByKey, const TArray<class UActionCombatStyleTransitionGraphNode*>& TransitionNodes);

private:
    UPROPERTY(Transient)
    TObjectPtr<UActionCombatStyleData> StyleData = nullptr;

    int32 ActionCount = 0;
    int32 TransitionCount = 0;
    int32 ReachableActionNodeCount = 0;
    int32 MissingDefinitionCount = 0;
};

UCLASS()
class UActionCombatStyleActionGraphNode : public UEdGraphNode
{
    GENERATED_BODY()

public:
    static const FName InputPinName;
    static const FName OutputPinName;

    UPROPERTY(Transient)
    int32 ActionIndex = INDEX_NONE;

    UPROPERTY(Transient)
    FGameplayTag ActionTag;

    UPROPERTY(Transient)
    bool bRootNode = false;

    UPROPERTY(Transient)
    bool bMissingDefinition = false;

    UPROPERTY(Transient)
    FString Subtitle;

    virtual void AllocateDefaultPins() override;
    virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
    virtual FText GetTooltipText() const override;
    virtual FLinearColor GetNodeTitleColor() const override;
    virtual FLinearColor GetNodeBodyTintColor() const override;
    virtual bool CanUserDeleteNode() const override;

    UEdGraphPin* GetInputPin() const;
    UEdGraphPin* GetOutputPin() const;
    FString GetStableKey() const;
};

UCLASS()
class UActionCombatStyleTransitionGraphNode : public UEdGraphNode
{
    GENERATED_BODY()

public:
    static const FName InputPinName;
    static const FName OutputPinName;

    UPROPERTY(Transient)
    int32 TransitionIndex = INDEX_NONE;

    UPROPERTY(Transient)
    FString Label;

    UPROPERTY(Transient)
    FGameplayTag FromActionTag;

    UPROPERTY(Transient)
    FGameplayTag ToActionTag;

    virtual void AllocateDefaultPins() override;
    virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
    virtual FText GetTooltipText() const override;
    virtual FLinearColor GetNodeTitleColor() const override;
    virtual FLinearColor GetNodeBodyTintColor() const override;
    virtual bool CanUserDeleteNode() const override;

    UEdGraphPin* GetInputPin() const;
    UEdGraphPin* GetOutputPin() const;
    FString GetStableKey() const;
};

USTRUCT()
struct FActionCombatGraphSchemaAction_AddActionNode : public FEdGraphSchemaAction
{
    GENERATED_BODY()

    FActionCombatGraphSchemaAction_AddActionNode()
        : FEdGraphSchemaAction()
    {
    }

    FActionCombatGraphSchemaAction_AddActionNode(FText InNodeCategory, FText InMenuDesc, FText InToolTip, int32 InGrouping)
        : FEdGraphSchemaAction(MoveTemp(InNodeCategory), MoveTemp(InMenuDesc), MoveTemp(InToolTip), InGrouping)
    {
    }

    virtual UEdGraphNode* PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode = true) override;
};

UCLASS()
class UActionCombatStyleGraphSchema : public UEdGraphSchema
{
    GENERATED_BODY()

public:
    static const FName PC_Flow;

    virtual void GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const override;
    virtual const FPinConnectionResponse CanCreateConnection(const UEdGraphPin* PinA, const UEdGraphPin* PinB) const override;
    virtual bool TryCreateConnection(UEdGraphPin* PinA, UEdGraphPin* PinB) const override;
    virtual bool CreateAutomaticConversionNodeAndConnections(UEdGraphPin* PinA, UEdGraphPin* PinB) const override;
    virtual FLinearColor GetPinTypeColor(const FEdGraphPinType& PinType) const override;
    virtual EGraphType GetGraphType(const UEdGraph* TestEdGraph) const override;
    virtual void DroppedAssetsOnNode(const TArray<FAssetData>& Assets, const FVector2D& GraphPosition, UEdGraphNode* Node) const override;
    virtual void GetAssetsNodeHoverMessage(const TArray<FAssetData>& Assets, const UEdGraphNode* HoverNode, FString& OutTooltipText, bool& OutOkIcon) const override;
    virtual FConnectionDrawingPolicy* CreateConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float InZoomFactor, const FSlateRect& InClippingRect, FSlateWindowElementList& InDrawElements, UEdGraph* InGraphObj) const override;
};
