// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SmoresCharacters : ModuleRules
{
	public SmoresCharacters(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"AIModule",
			"NavigationSystem",
			"SmoresCore",
			"SmoresItems",
			"SmoresCombat"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"SmoresCharacters"
		});
	}
}
