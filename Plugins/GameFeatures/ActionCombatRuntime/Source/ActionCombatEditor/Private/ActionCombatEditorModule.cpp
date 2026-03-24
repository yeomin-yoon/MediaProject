#include "Modules/ModuleManager.h"

#include "AssetToolsModule.h"
#include "IAssetTools.h"

#include "AssetTypeActions_ActionCombatStyleData.h"

class FActionCombatEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

        StyleAssetTypeActions = MakeShared<FAssetTypeActions_ActionCombatStyleData>();
        AssetTools.RegisterAssetTypeActions(StyleAssetTypeActions.ToSharedRef());
    }

    virtual void ShutdownModule() override
    {
        if (FModuleManager::Get().IsModuleLoaded("AssetTools") && StyleAssetTypeActions.IsValid())
        {
            IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
            AssetTools.UnregisterAssetTypeActions(StyleAssetTypeActions.ToSharedRef());
        }

        StyleAssetTypeActions.Reset();
    }

private:
    TSharedPtr<FAssetTypeActions_ActionCombatStyleData> StyleAssetTypeActions;
};

IMPLEMENT_MODULE(FActionCombatEditorModule, ActionCombatEditor)
