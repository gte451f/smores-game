// Copyright 2026 Jim Jenkins. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class smoresEditorTarget : TargetRules
{
	public smoresEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.Add("smores");
		ExtraModuleNames.Add("SmoresCore");
		ExtraModuleNames.Add("SmoresCombat");
		ExtraModuleNames.Add("SmoresItems");
		ExtraModuleNames.Add("SmoresCharacters");
		ExtraModuleNames.Add("SmoresUI");
		ExtraModuleNames.Add("SmoresEconomy");
		ExtraModuleNames.Add("SmoresDialog");
		// THROWAWAY: the conversation-player spike, removed once Jim picks Ink or Yarn
		ExtraModuleNames.Add("SmoresDialogSpike");
	}
}
