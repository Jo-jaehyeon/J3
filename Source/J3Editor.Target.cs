// Copyright (c) ACCENT. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class J3EditorTarget : TargetRules
{
	public J3EditorTarget( TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.Add("J3");
	}
}
