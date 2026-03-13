using UnrealBuildTool;

public class LockOnSystemLyra : ModuleRules
{
    public LockOnSystemLyra(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "LockOnSystemRuntime",
                "LyraGame"
            });

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "EnhancedInput",
                "InputCore",
                "ModularGameplay"
            });
    }
}
