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
                "GameplayTags"
            });

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "GameplayTasks"
            });
    }
}
