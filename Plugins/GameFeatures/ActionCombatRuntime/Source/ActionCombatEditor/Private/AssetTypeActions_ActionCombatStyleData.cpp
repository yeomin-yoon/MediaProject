#include "AssetTypeActions_ActionCombatStyleData.h"

#include "ActionCombatStyleData.h"
#include "ActionCombatStyleAssetEditor.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions_ActionCombatStyleData"

FText FAssetTypeActions_ActionCombatStyleData::GetName() const
{
    return LOCTEXT("ActionCombatStyleDataName", "Action Combat Style Data");
}

FColor FAssetTypeActions_ActionCombatStyleData::GetTypeColor() const
{
    return FColor(208, 111, 55);
}

UClass* FAssetTypeActions_ActionCombatStyleData::GetSupportedClass() const
{
    return UActionCombatStyleData::StaticClass();
}

uint32 FAssetTypeActions_ActionCombatStyleData::GetCategories()
{
    return EAssetTypeCategories::Gameplay;
}

void FAssetTypeActions_ActionCombatStyleData::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
    const EToolkitMode::Type ToolkitMode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;

    for (UObject* Object : InObjects)
    {
        UActionCombatStyleData* StyleData = Cast<UActionCombatStyleData>(Object);
        if (!StyleData)
        {
            continue;
        }

        TSharedRef<FActionCombatStyleAssetEditor> Editor = MakeShared<FActionCombatStyleAssetEditor>();
        Editor->InitStyleAssetEditor(ToolkitMode, EditWithinLevelEditor, StyleData);
    }
}

#undef LOCTEXT_NAMESPACE
