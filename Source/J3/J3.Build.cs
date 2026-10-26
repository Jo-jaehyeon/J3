// Copyright (c) ACCENT. All Rights Reserved.

using UnrealBuildTool;

public class J3 : ModuleRules
{
	public J3(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// "Data/J3DataTypes.h"처럼 모듈 루트 기준으로 include 한다
		PublicIncludePaths.Add(ModuleDirectory);

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "NetCore", "GameplayAbilities", "GameplayTags", "GameplayTasks" });

		PrivateDependencyModuleNames.AddRange(new string[] { "UMG", "Slate", "SlateCore", "DeveloperSettings", "Landscape" });

		// 복제는 처음부터 Iris로 검증한다 (Multiplayer_Architecture 4.2.2). 실제 사용 여부는 net.Iris.UseIrisReplication
		SetupIrisSupport(Target);

		// 에디터에서 Data/*.csv 변경을 감시해 다시 읽는다
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.Add("DirectoryWatcher");
		}

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
