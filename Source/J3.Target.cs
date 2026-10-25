// Copyright (c) ACCENT. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class J3Target : TargetRules
{
	public J3Target(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.Add("J3");
	}
}
