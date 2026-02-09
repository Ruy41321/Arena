// Copyright (c) 2025 Luigi Pennisi. All rights reserved.

#include "Player/MovementStateMachine/States/DodgingMovementState.h"
#include "Player/Components/Dodge/DodgeSystemComponent.h"
#include "Player/Components/Sprint/SprintSystemComponent.h"
#include "Player/Components/Crouch/CrouchSystemComponent.h"
#include "Player/PlayerCharacter.h"
#include "InputActionValue.h"
#include "GameFramework/CharacterMovementComponent.h"

UDodgingMovementState::UDodgingMovementState()
{
}

EMovementStateValue UDodgingMovementState::GetDesiredTransition_Implementation() const
{
	APlayerCharacter* Player = GetPlayerCharacter();
	if (!Player || !Player->GetCharacterMovement())
		return EMovementStateValue::None;

	// Check if no longer dodging
	if (!Player->DodgeSystem || !Player->DodgeSystem->IsDodging())
	{
		// Check for falling
		if (Player->GetCharacterMovement()->IsFalling())
		{
			FVector Velocity = Player->GetCharacterMovement()->Velocity;
			if (Velocity.Z > 0.0f)
				return EMovementStateValue::Jumping;
			else
				return EMovementStateValue::Falling;
		}

		// Determine ground state based on crouching and velocity
		if (Player->CrouchSystem && Player->CrouchSystem->IsCrouched() && !(!Player->SprintSystem->IsSprintInterrupted() && Player->CrouchSystem->CanUncrouchSafely()))
		{
			float HorizontalSpeed = Player->GetCharacterMovement()->Velocity.Size2D();
			if (HorizontalSpeed > 0.1f)
				return EMovementStateValue::CrouchingMoving;
			else
				return EMovementStateValue::CrouchingIdle;
		}
		
		float Speed = Player->GetCharacterMovement()->Velocity.Size2D();
		if (Speed > 10.0f)
		{
			if (Player->SprintSystem && !Player->SprintSystem->IsSprintInterrupted())
			{
				Player->SprintSystem->SetIsSprinting(true);
				if (Player->CrouchSystem && Player->CrouchSystem->IsCrouched())
					Player->CrouchSystem->CrouchPressed(FInputActionValue());
				return EMovementStateValue::Sprinting;
			}
			else
				return EMovementStateValue::Walking;
		}
		else
			return EMovementStateValue::Idle;
	}

	return EMovementStateValue::None;
}

bool UDodgingMovementState::CanTransitionTo_Implementation(EMovementStateValue NewState) const
{
	// Dodging can transition to any state once completed
	switch (NewState)
	{
	case EMovementStateValue::CrouchingIdle:
	case EMovementStateValue::CrouchingMoving:
		// Can return to crouching states
		return true;
	case EMovementStateValue::Idle:
	case EMovementStateValue::Walking:
	case EMovementStateValue::Sprinting:
		// Basic movement transitions allowed
		return true;
	case EMovementStateValue::Jumping:
	case EMovementStateValue::Falling:
		// Air movement always allowed
		return true;
	case EMovementStateValue::LandingInPlace:
	case EMovementStateValue::LandingMoving:
		// Landing can be reached if ending in air
		return true;
	default:
		// Other states use base logic
		return Super::CanTransitionTo_Implementation(NewState);
	}
}
