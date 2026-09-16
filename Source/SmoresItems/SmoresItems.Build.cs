// Copyright 2026 Jim Jenkins. All Rights Reserved.

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

		// AssetRegistry is here only for Tests/ItemDefinitionAssetTest.cpp, which sweeps every
		// UItemDefinition asset under Content/. Nothing in the module's own runtime code uses it.
		PrivateDependencyModuleNames.AddRange(new string[] {
			"AssetRegistry"
		});

		PublicIncludePaths.AddRange(new string[] {
			"SmoresItems"
		});
	}
}
