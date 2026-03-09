// Copyright (c) 2025 Luigi Pennisi. All rights reserved.

#include "Player/MovementStateMachine/States/CrouchingIdleMovementState.h"
#include "Player/Components/Dodge/DodgeSystemComponent.h"
#include "Player/Components/Crouch/CrouchSystemComponent.h"
#include "Player/PlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

UCrouchingIdleMovementState::UCrouchingIdleMovementState()
{
}

EMovementStateValue UCrouchingIdleMovementState::GetDesiredTransition_Implementation() const
{
	APlayerCharacter* Player = GetPlayerCharacter();
	if (!Player || !Player->GetCharacterMovement())
		return EMovementStateValue::None;

	// Check for dodging - high priority
	if (Player->DodgeSystem && Player->DodgeSystem->IsDodging())
		return EMovementStateValue::Dodging;

	// Check for falling
	if (Player->GetCharacterMovement()->IsFalling())
	{
		FVector Velocity = Player->GetCharacterMovement()->Velocity;
		if (Velocity.Z > 0.0f)
			return EMovementStateValue::Jumping;
		else
			return EMovementStateValue::Falling;
	}

	// Check if no longer crouching
	if (!Player->CrouchSystem || !Player->CrouchSystem->IsCrouched())
	{
		float Speed = Player->GetCharacterMovement()->Velocity.Size2D();
		if (Speed > 10.0f)
			return EMovementStateValue::Walking;
		else
			return EMovementStateValue::Idle;
	}

	// Check if moving while crouched
	float HorizontalSpeed = Player->GetCharacterMovement()->Velocity.Size2D();
	if (HorizontalSpeed > 0.1f) // Small threshold to avoid floating-point precision issues
	{
		return EMovementStateValue::CrouchingMoving;
	}

	return EMovementStateValue::None;
}

bool UCrouchingIdleMovementState::CanTransitionTo_Implementation(EMovementStateValue NewState) const
{
	// Crouching Idle can transition to movement states
	switch (NewState)
	{
	case EMovementStateValue::CrouchingMoving:
		// Can transition to crouching movement
		return true;
	case EMovementStateValue::Dodging:
		// Can dodge while crouched idle
		return true;
	case EMovementStateValue::Jumping:
	case EMovementStateValue::Falling:
		// Air movement always allowed
		return true;
	case EMovementStateValue::Idle:
	case EMovementStateValue::Walking:
	case EMovementStateValue::Sprinting:
		// Stand up transitions
		return true;
	case EMovementStateValue::LandingInPlace:
	case EMovementStateValue::LandingMoving:
		// Landing states can be reached from air
		return true;
	default:
		return Super::CanTransitionTo_Implementation(NewState);
	}
}