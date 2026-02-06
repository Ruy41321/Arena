// Copyright (c) 2025 Luigi Pennisi. All rights reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class ArenaEditorTarget : TargetRules
{
    public ArenaEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		
		// 1. Usa la V6 per UE 5.7
		DefaultBuildSettings = BuildSettingsVersion.V6;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;

		// 2. IMPORTANTE: Rimuovi o commenta la riga "Unique"
		// BuildEnvironment = TargetBuildEnvironment.Unique; 

		// 3. Forza il superamento del controllo sugli identificatori non definiti
		// Questo permette di usare l'ambiente condiviso senza l'errore precedente
		bOverrideBuildEnvironment = true;

		ExtraModuleNames.AddRange( new string[] { "Arena" } );
	}
}
