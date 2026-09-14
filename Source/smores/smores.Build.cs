// Copyright 2026 Jim Jenkins. All Rights Reserved.

using UnrealBuildTool;

public class smores : ModuleRules
{
	public smores(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"NavigationSystem",
			"Niagara",
			"UMG",
			"Slate",
			"SlateCore",
			"SmoresCore",
			"SmoresCombat",
			"SmoresItems",
			"SmoresCharacters",
			"SmoresUI",
			"SmoresEconomy"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"smores",
			"smores/Variant_Strategy",
			"smores/MainMenu",
			"smores/MainMenu/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
