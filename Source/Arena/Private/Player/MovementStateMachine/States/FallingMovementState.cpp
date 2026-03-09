// Copyright (c) 2025 Luigi Pennisi. All rights reserved.

#include "Player/MovementStateMachine/States/FallingMovementState.h"
#include "Player/PlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

UFallingMovementState::UFallingMovementState()
{
}

EMovementStateValue UFallingMovementState::GetDesiredTransition_Implementation() const
{
	APlayerCharacter* Player = GetPlayerCharacter();
	if (!Player || !Player->GetCharacterMovement())
		return EMovementStateValue::None;

	// Check if we've landed
	if (!Player->GetCharacterMovement()->IsFalling())
	{
		// Decide between LandingInPlace and LandingMoving based on horizontal velocity
		float HorizontalSpeed = Player->GetCharacterMovement()->Velocity.Size2D();
		if (HorizontalSpeed > 0.1f) // Small threshold to avoid floating-point precision issues
			return EMovementStateValue::LandingMoving;
		else
			return EMovementStateValue::LandingInPlace;
	}

	return EMovementStateValue::None;
}
