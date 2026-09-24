// Copyright 2026 Jim Jenkins. All Rights Reserved.

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

		// AssetRegistry is here only for Tests/CharacterDefinitionAssetTest.cpp, which sweeps every
		// UCharacterDefinition asset under Content/. Nothing in the module's own runtime code uses it.
		PrivateDependencyModuleNames.AddRange(new string[] {
			"AssetRegistry"
		});

		PublicIncludePaths.AddRange(new string[] {
			"SmoresCharacters"
		});
	}
}
