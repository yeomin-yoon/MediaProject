#pragma once

#include "Widgets/SCompoundWidget.h"

class UActionCombatStyleData;

class SActionCombatStyleGraphView : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SActionCombatStyleGraphView)
    {
    }

        SLATE_ARGUMENT(UActionCombatStyleData*, StyleData)

    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);
    void RefreshGraph();

private:
    TWeakObjectPtr<UActionCombatStyleData> StyleData;
    TSharedPtr<class STextBlock> SummaryTextBlock;
    TSharedPtr<class SBox> GraphHost;
};
