// Copyright (c) 2025 Luigi Pennisi. All rights reserved.

#include "Player/MovementStateMachine/States/JumpingMovementState.h"
#include "Player/Components/Jump/JumpSystemComponent.h"
#include "Player/Components/Sprint/SprintSystemComponent.h"
#include "Player/Components/Crouch/CrouchSystemComponent.h"
#include "Player/PlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

UJumpingMovementState::UJumpingMovementState()
{
}

EMovementState UJumpingMovementState::GetDesiredTransition_Implementation() const
{
	APlayerCharacter* Player = GetPlayerCharacter();
	if (!Player || !Player->GetCharacterMovement())
		return EMovementState::None;

	// Check if we're falling (velocity.Z <= 0)
	if (Player->GetCharacterMovement()->IsFalling())
	{
		FVector Velocity = Player->GetCharacterMovement()->Velocity;
		if (Velocity.Z <= 0.0f)
			return EMovementState::Falling;
	}
	else
	{
		// We've landed, check for landing state
		if (Player->JumpSystem && Player->JumpSystem->IsLanding())
		{
			// Decide between LandingInPlace and LandingMoving based on horizontal velocity
			float HorizontalSpeed = Player->GetCharacterMovement()->Velocity.Size2D();
			if (HorizontalSpeed > 0.1f) // Small threshold to avoid floating-point precision issues
				return EMovementState::LandingMoving;
			else
				return EMovementState::LandingInPlace;
		}
		
		// Determine ground state based on other conditions - decide between crouching states based on velocity
		if (Player->CrouchSystem && Player->CrouchSystem->IsCrouched())
		{
			float HorizontalSpeed = Player->GetCharacterMovement()->Velocity.Size2D();
			if (HorizontalSpeed > 0.1f)
				return EMovementState::CrouchingMoving;
			else
				return EMovementState::CrouchingIdle;
		}
		
		float Speed = Player->GetCharacterMovement()->Velocity.Size2D();
		if (Speed > 10.0f)
		{
			if (Player->SprintSystem && Player->SprintSystem->IsSprinting())
				return EMovementState::Sprinting;
			else
				return EMovementState::Walking;
		}
		else
			return EMovementState::Idle;
	}

	return EMovementState::None;
}
