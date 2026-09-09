// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SmoresCore : ModuleRules
{
	public SmoresCore(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"SmoresCore"
		});
	}
}
