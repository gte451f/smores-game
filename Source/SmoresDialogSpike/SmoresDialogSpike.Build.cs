// Copyright 2026 Jim Jenkins. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

/**
 *  THROWAWAY - the conversation-player spike (dialog roadmap, before Slice 3).
 *
 *  Plays one scene written in both Ink and Yarn, loaded from loose files under Mods/, so Jim can
 *  choose a language from evidence. Once he has, the loser's half is deleted and the winner's
 *  player moves into SmoresDialog in Slice 3; this module then goes away.
 *
 *  inkcpp's own sources compile inside this module as plain C++, which is why the settings below
 *  are looser than any other module's: no shared precompiled header is forced into them, each file
 *  builds on its own, and exceptions are on because inkcpp's JSON converter reports errors by
 *  throwing.
 */
public class SmoresDialogSpike : ModuleRules
{
	public SmoresDialogSpike(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.NoPCHs;
		bUseUnity = false;
		bEnableExceptions = true;
		CppCompileWarningSettings.ShadowVariableWarningLevel = WarningLevel.Off;
		CppCompileWarningSettings.UndefinedIdentifierWarningLevel = WarningLevel.Off;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"SmoresCore",
			"SmoresDialog",
			"YarnSpinner"
		});

		string Ink = Path.Combine(ModuleDirectory, "ThirdParty", "inkcpp");

		PrivateIncludePaths.AddRange(new string[] {
			ModuleDirectory,
			Path.Combine(Ink, "inkcpp", "include"),
			Path.Combine(Ink, "shared", "public"),
			Path.Combine(Ink, "shared", "private"),
			Path.Combine(Ink, "inkcpp_compiler", "include")
		});

		// inkcpp's plain-C++ build, minus RTTI (Unreal compiles with it off). INK_EXPOSE_JSON is what
		// inkcpp's own build sets for its converter.
		PrivateDefinitions.AddRange(new string[] {
			"INKCPP_NO_RTTI",
			"INK_EXPOSE_JSON"
		});
	}
}
