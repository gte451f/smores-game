// Copyright 2026 Jim Jenkins. All Rights Reserved.

using UnrealBuildTool;

public class SmoresDialog : ModuleRules
{
	public SmoresDialog(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Dialog reads nearly everything below it and almost nothing reads dialog - see
		// unreal-module-organization.md. SmoresEconomy joins this list with the first effect that
		// touches a wallet (dialog Slice 3), not before.
		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"SmoresCore",
			"SmoresCombat",
			"SmoresCharacters"
		});

		// Json reads each package's mod.json manifest. Nothing outside the loader needs it.
		PrivateDependencyModuleNames.AddRange(new string[] {
			"Json"
		});

		PublicIncludePaths.AddRange(new string[] {
			"SmoresDialog"
		});
	}
}
