// Copyright (c) 2025 Luigi Pennisi. All rights reserved.

#include "DodgingMovementState.h"
#include "../../PlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

UDodgingMovementState::UDodgingMovementState()
{
}

EMovementState UDodgingMovementState::GetDesiredTransition_Implementation() const
{
	APlayerCharacter* Player = GetPlayerCharacter();
	if (!Player || !Player->GetCharacterMovement())
		return EMovementState::None;

	// Check if no longer dodging
	if (!Player->DodgeSystem || !Player->DodgeSystem->IsDodging())
	{
		// Check for falling
		if (Player->GetCharacterMovement()->IsFalling())
		{
			FVector Velocity = Player->GetCharacterMovement()->Velocity;
			if (Velocity.Z > 0.0f)
				return EMovementState::Jumping;
			else
				return EMovementState::Falling;
		}

		// Determine ground state based on crouching and velocity
		if (Player->CrouchSystem && Player->CrouchSystem->IsCrouched() && !(!Player->SprintSystem->IsSprintInterrupted() && Player->CrouchSystem->CanUncrouchSafely()))
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
			if (Player->SprintSystem && !Player->SprintSystem->IsSprintInterrupted())
			{
				Player->SprintSystem->SetIsSprinting(true);
				if (Player->CrouchSystem && Player->CrouchSystem->IsCrouched())
					Player->CrouchSystem->CrouchPressed(NULL);
				return EMovementState::Sprinting;
			}
			else
				return EMovementState::Walking;
		}
		else
			return EMovementState::Idle;
	}

	return EMovementState::None;
}

bool UDodgingMovementState::CanTransitionTo_Implementation(EMovementState NewState) const
{
	// Dodging can transition to any state once completed
	switch (NewState)
	{
	case EMovementState::CrouchingIdle:
	case EMovementState::CrouchingMoving:
		// Can return to crouching states
		return true;
	case EMovementState::Idle:
	case EMovementState::Walking:
	case EMovementState::Sprinting:
		// Basic movement transitions allowed
		return true;
	case EMovementState::Jumping:
	case EMovementState::Falling:
		// Air movement always allowed
		return true;
	case EMovementState::LandingInPlace:
	case EMovementState::LandingMoving:
		// Landing can be reached if ending in air
		return true;
	default:
		// Other states use base logic
		return Super::CanTransitionTo_Implementation(NewState);
	}
}
