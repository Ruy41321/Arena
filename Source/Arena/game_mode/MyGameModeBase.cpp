// Fill out your copyright notice in the Description page of Project Settings.


#include "MyGameModeBase.h"
#include "../Player/PlayerCharacter.h"

AMyGameModeBase::AMyGameModeBase()
{
	// Set this game mode to be the default for the project
	DefaultPawnClass = APlayerCharacter::StaticClass();
	// You can set other properties here, such as GameStateClass, PlayerControllerClass, etc.
}
