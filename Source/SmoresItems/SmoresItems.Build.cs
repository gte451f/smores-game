// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SmoresItems : ModuleRules
{
	public SmoresItems(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"SmoresCore"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"SmoresItems"
		});
	}
}
