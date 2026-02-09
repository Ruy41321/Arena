// Copyright (c) 2025 Luigi Pennisi. All rights reserved.

#include "Player/MovementStateMachine/States/WalkingMovementState.h"
#include "Player/PlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

UWalkingMovementState::UWalkingMovementState()
{
}

EMovementState UWalkingMovementState::GetDesiredTransition_Implementation() const
{
	APlayerCharacter* Player = GetPlayerCharacter();
	if (!Player || !Player->GetCharacterMovement())
		return EMovementState::None;

	// Check for falling first
	if (Player->GetCharacterMovement()->IsFalling())
	{
		FVector Velocity = Player->GetCharacterMovement()->Velocity;
		if (Velocity.Z > 0.0f)
			return EMovementState::Jumping;
		else
			return EMovementState::Falling;
	}

	// Check for dodging
	if (Player->DodgeSystem && Player->DodgeSystem->IsDodging())
		return EMovementState::Dodging;

	// Check for crouching - decide between idle and moving based on velocity
	if (Player->CrouchSystem && Player->CrouchSystem->IsCrouched())
	{
		float HorizontalSpeed = Player->GetCharacterMovement()->Velocity.Size2D();
		if (HorizontalSpeed > 0.1f)
			return EMovementState::CrouchingMoving;
		else
			return EMovementState::CrouchingIdle;
	}

	// Check movement speed
	float Speed = Player->GetCharacterMovement()->Velocity.Size2D();
	
	// If no movement, go to idle
	if (Speed <= 10.0f)
		return EMovementState::Idle;

	// Check if sprinting
	if (Player->SprintSystem && Player->SprintSystem->IsSprinting())
		return EMovementState::Sprinting;

	// Stay in walking state
	return EMovementState::None;
}
