using UnrealBuildTool;

public class ActionCombatRuntime : ModuleRules
{
    public ActionCombatRuntime(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "GameplayAbilities",
                "GameplayTags",
                "NetCore"
            });

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "GameplayTasks",
                "Projects"
            });
    }
}
