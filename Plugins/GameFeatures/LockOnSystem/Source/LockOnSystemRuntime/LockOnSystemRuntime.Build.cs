using UnrealBuildTool;

public class LockOnSystemRuntime : ModuleRules
{
    public LockOnSystemRuntime(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "DeveloperSettings",
                "Engine",
                "GameplayTags"
            });

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "ModularGameplay",
                "NetCore"
            });
    }
}
