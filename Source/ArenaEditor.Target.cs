// Copyright (c) 2025 Luigi Pennisi. All rights reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class ArenaEditorTarget : TargetRules
{
	public ArenaEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V5;

		ExtraModuleNames.AddRange( new string[] { "Arena" } );
	}
}
