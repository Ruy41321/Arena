// Copyright (c) 2025 Luigi Pennisi. All rights reserved.


#include "MyGameModeBase.h"
#include "../Player/PlayerCharacter.h"

AMyGameModeBase::AMyGameModeBase()
{
	// Set this game mode to be the default for the project
	DefaultPawnClass = APlayerCharacter::StaticClass();
	// You can set other properties here, such as GameStateClass, PlayerControllerClass, etc.
}
