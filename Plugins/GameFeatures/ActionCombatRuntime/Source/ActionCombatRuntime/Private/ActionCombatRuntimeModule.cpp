#include "GameplayTagsManager.h"
#include "Interfaces/IPluginManager.h"
#include "Modules/ModuleManager.h"

class FActionCombatRuntimeModule final : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

void FActionCombatRuntimeModule::StartupModule()
{
	const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("ActionCombatRuntime"));
	if (!Plugin.IsValid())
	{
		return;
	}

	UGameplayTagsManager::Get().AddTagIniSearchPath(Plugin->GetBaseDir() / TEXT("Config/Tags"));
}

void FActionCombatRuntimeModule::ShutdownModule()
{
}

IMPLEMENT_MODULE(FActionCombatRuntimeModule, ActionCombatRuntime)
