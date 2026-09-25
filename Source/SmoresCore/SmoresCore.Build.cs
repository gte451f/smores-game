// Copyright 2026 Jim Jenkins. All Rights Reserved.

using UnrealBuildTool;

public class SmoresCore : ModuleRules
{
	public SmoresCore(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			// public: USmoresDefinition::Tags puts an FGameplayTagContainer in a header every
			// module that authors a definition includes
			"GameplayTags"
		});

		// AssetRegistry is here only for Tests/SmoresDefinitionAssetTest.cpp, which sweeps every
		// USmoresDefinition asset under Content/. Nothing in the module's own runtime code uses it.
		PrivateDependencyModuleNames.AddRange(new string[] {
			"AssetRegistry"
		});

		PublicIncludePaths.AddRange(new string[] {
			"SmoresCore"
		});
	}
}
