// Copyright (c) 2025 Luigi Pennisi. All rights reserved.

#include "Player/MovementStateMachine/States/CrouchingMovingMovementState.h"
#include "Player/Components/Dodge/DodgeSystemComponent.h"
#include "Player/Components/Sprint/SprintSystemComponent.h"
#include "Player/Components/Crouch/CrouchSystemComponent.h"
#include "Player/PlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

UCrouchingMovingMovementState::UCrouchingMovingMovementState()
{
}

EMovementState UCrouchingMovingMovementState::GetDesiredTransition_Implementation() const
{
	APlayerCharacter* Player = GetPlayerCharacter();
	if (!Player || !Player->GetCharacterMovement())
		return EMovementState::None;

	// Check for dodging - high priority
	if (Player->DodgeSystem && Player->DodgeSystem->IsDodging())
		return EMovementState::Dodging;

	// Check for falling
	if (Player->GetCharacterMovement()->IsFalling())
	{
		FVector Velocity = Player->GetCharacterMovement()->Velocity;
		if (Velocity.Z > 0.0f)
			return EMovementState::Jumping;
		else
			return EMovementState::Falling;
	}

	// Check if no longer crouching
	if (!Player->CrouchSystem || !Player->CrouchSystem->IsCrouched())
	{
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

	// Check if stopped moving while crouched
	float HorizontalSpeed = Player->GetCharacterMovement()->Velocity.Size2D();
	if (HorizontalSpeed <= 0.1f) // Small threshold to avoid floating-point precision issues
	{
		return EMovementState::CrouchingIdle;
	}

	return EMovementState::None;
}

bool UCrouchingMovingMovementState::CanTransitionTo_Implementation(EMovementState NewState) const
{
	// Crouching Moving can transition to various states
	switch (NewState)
	{
	case EMovementState::CrouchingIdle:
		// Can stop while crouched
		return true;
	case EMovementState::Dodging:
		// Can dodge while crouched moving
		return true;
	case EMovementState::Jumping:
	case EMovementState::Falling:
		// Air movement always allowed
		return true;
	case EMovementState::Walking:
	case EMovementState::Sprinting:
		// Stand up and continue moving
		return true;
	case EMovementState::Idle:
		// Stand up and stop
		return true;
	case EMovementState::LandingInPlace:
	case EMovementState::LandingMoving:
		// Landing states can be reached from air
		return true;
	default:
		return Super::CanTransitionTo_Implementation(NewState);
	}
}