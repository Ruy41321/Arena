// Copyright (c) 2025 Luigi Pennisi. All rights reserved.

#include "CrouchingIdleMovementState.h"
#include "../../PlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

UCrouchingIdleMovementState::UCrouchingIdleMovementState()
{
}

EMovementState UCrouchingIdleMovementState::GetDesiredTransition_Implementation() const
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
			return EMovementState::Walking;
		else
			return EMovementState::Idle;
	}

	// Check if moving while crouched
	float HorizontalSpeed = Player->GetCharacterMovement()->Velocity.Size2D();
	if (HorizontalSpeed > 0.1f) // Small threshold to avoid floating-point precision issues
	{
		return EMovementState::CrouchingMoving;
	}

	return EMovementState::None;
}

bool UCrouchingIdleMovementState::CanTransitionTo_Implementation(EMovementState NewState) const
{
	// Crouching Idle can transition to movement states
	switch (NewState)
	{
	case EMovementState::CrouchingMoving:
		// Can transition to crouching movement
		return true;
	case EMovementState::Dodging:
		// Can dodge while crouched idle
		return true;
	case EMovementState::Jumping:
	case EMovementState::Falling:
		// Air movement always allowed
		return true;
	case EMovementState::Idle:
	case EMovementState::Walking:
	case EMovementState::Sprinting:
		// Stand up transitions
		return true;
	case EMovementState::LandingInPlace:
	case EMovementState::LandingMoving:
		// Landing states can be reached from air
		return true;
	default:
		return Super::CanTransitionTo_Implementation(NewState);
	}
}