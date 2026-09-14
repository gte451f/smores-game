// Copyright 2026 Jim Jenkins. All Rights Reserved.

using UnrealBuildTool;

public class SmoresEconomy : ModuleRules
{
	public SmoresEconomy(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"SmoresCore",
			"SmoresItems"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"SmoresEconomy"
		});
	}
}
