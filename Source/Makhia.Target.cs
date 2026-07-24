// Copyright (c) 2025 Luigi Pennisi. All rights reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class MakhiaTarget : TargetRules
{
	public MakhiaTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;

		ExtraModuleNames.AddRange( new string[] { "Makhia" } );
	}
}
