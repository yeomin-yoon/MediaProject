using UnrealBuildTool;

public class ActionCombatLyraBridge : ModuleRules
{
    public ActionCombatLyraBridge(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "ActionCombatRuntime",
                "Core",
                "CoreUObject",
                "Engine",
                "GameplayAbilities",
                "GameplayTasks",
                "GameplayTags",
                "InputCore",
                "LyraGame"
            });

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "EnhancedInput",
                "LockOnSystemRuntime"
            });
    }
}
