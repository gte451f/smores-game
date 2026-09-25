// Copyright 2026 Jim Jenkins. All Rights Reserved.

using UnrealBuildTool;

/**
 *  THROWAWAY - the conversation-player spike (dialog roadmap, before Slice 3).
 *
 *  Plays the bandit-shakedown scene in Yarn from loose files under Mods/, through the Yarn Spinner
 *  plugin's player. Jim chose Yarn over Ink from it on 2026-09-25 (the Ink half is in git history,
 *  commit a466c73). Slice 3 moves the Yarn player into SmoresDialog and deletes this module.
 */
public class SmoresDialogSpike : ModuleRules
{
	public SmoresDialogSpike(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"SmoresCore",
			"SmoresDialog",
			"YarnSpinner"
		});

		PrivateIncludePaths.AddRange(new string[] {
			ModuleDirectory
		});
	}
}
