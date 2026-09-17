// Copyright 2026 Jim Jenkins. All Rights Reserved.

using UnrealBuildTool;

public class SmoresUI : ModuleRules
{
	public SmoresUI(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"UMG",
			"Slate",
			"SlateCore",
			"SmoresCore",
			"SmoresItems",
			// arrives transitively through SmoresCharacters, but the HUD reads UHealthComponent
			// directly (portrait rings, the target panel's health bar) - a real dependency that
			// shouldn't be hidden behind another module's include
			"SmoresCombat",
			"SmoresCharacters",
			"SmoresEconomy"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"SmoresUI"
		});
	}
}
