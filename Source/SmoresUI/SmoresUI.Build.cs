// Copyright Epic Games, Inc. All Rights Reserved.

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
			"SmoresCharacters"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"SmoresUI"
		});
	}
}
