// Copyright 2026 Jim Jenkins. All Rights Reserved.

using UnrealBuildTool;

public class SmoresDialog : ModuleRules
{
	public SmoresDialog(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Dialog reads nearly everything below it and almost nothing reads dialog - see
		// unreal-module-organization.md.
		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"SmoresCore",
			"SmoresCombat",
			"SmoresCharacters"
		});

		// Json reads each package's mod.json manifest. SmoresEconomy is the wallet the gold() fact
		// and the TakeMoney / GiveMoney effects reach.
		//
		// YarnSpinner is the conversation player - the Yarn Spinner for Unreal plugin, of which only
		// the player itself is used (dialog.md). **Private, and it must stay private**: no public
		// header here may include a Yarn header (ConversationScript.h is SmoresDialog's own), so
		// nothing else in the game links against the plugin or needs its include paths.
		PrivateDependencyModuleNames.AddRange(new string[] {
			"Json",
			"SmoresEconomy",
			"YarnSpinner"
		});

		PublicIncludePaths.AddRange(new string[] {
			"SmoresDialog"
		});
	}
}
