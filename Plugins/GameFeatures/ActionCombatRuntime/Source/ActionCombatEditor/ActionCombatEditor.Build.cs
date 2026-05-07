using UnrealBuildTool;

public class ActionCombatEditor : ModuleRules
{
    public ActionCombatEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "ActionCombatRuntime",
                "Core",
                "CoreUObject",
                "Engine"
            });

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "AssetTools",
                "EditorFramework",
                "GameplayTags",
                "GraphEditor",
                "InputCore",
                "PropertyEditor",
                "Slate",
                "SlateCore",
                "UnrealEd"
            });
    }
}
