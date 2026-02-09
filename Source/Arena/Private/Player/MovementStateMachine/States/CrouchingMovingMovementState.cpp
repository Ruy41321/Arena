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

EMovementStateValue UCrouchingMovingMovementState::GetDesiredTransition_Implementation() const
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
		{
			if (Player->SprintSystem && Player->SprintSystem->IsSprinting())
				return EMovementStateValue::Sprinting;
			else
				return EMovementStateValue::Walking;
		}
		else
			return EMovementStateValue::Idle;
	}

	// Check if stopped moving while crouched
	float HorizontalSpeed = Player->GetCharacterMovement()->Velocity.Size2D();
	if (HorizontalSpeed <= 0.1f) // Small threshold to avoid floating-point precision issues
	{
		return EMovementStateValue::CrouchingIdle;
	}

	return EMovementStateValue::None;
}

bool UCrouchingMovingMovementState::CanTransitionTo_Implementation(EMovementStateValue NewState) const
{
	// Crouching Moving can transition to various states
	switch (NewState)
	{
	case EMovementStateValue::CrouchingIdle:
		// Can stop while crouched
		return true;
	case EMovementStateValue::Dodging:
		// Can dodge while crouched moving
		return true;
	case EMovementStateValue::Jumping:
	case EMovementStateValue::Falling:
		// Air movement always allowed
		return true;
	case EMovementStateValue::Walking:
	case EMovementStateValue::Sprinting:
		// Stand up and continue moving
		return true;
	case EMovementStateValue::Idle:
		// Stand up and stop
		return true;
	case EMovementStateValue::LandingInPlace:
	case EMovementStateValue::LandingMoving:
		// Landing states can be reached from air
		return true;
	default:
		return Super::CanTransitionTo_Implementation(NewState);
	}
}