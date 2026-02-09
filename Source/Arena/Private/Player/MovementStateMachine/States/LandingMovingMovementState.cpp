// Copyright (c) 2025 Luigi Pennisi. All rights reserved.

#include "Player/MovementStateMachine/States/LandingMovingMovementState.h"
#include "Player/Components/Jump/JumpSystemComponent.h"
#include "Player/Components/Sprint/SprintSystemComponent.h"
#include "Player/Components/Crouch/CrouchSystemComponent.h"
#include "Player/PlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

ULandingMovingMovementState::ULandingMovingMovementState()
{
}

EMovementState ULandingMovingMovementState::GetDesiredTransition_Implementation() const
{
	APlayerCharacter* Player = GetPlayerCharacter();
	if (!Player || !Player->GetCharacterMovement())
		return EMovementState::None;

	// Check if landing state is finished
	if (!Player->JumpSystem || !Player->JumpSystem->IsLanding())
	{
		// Landing finished - transition based on current conditions
		if (Player->CrouchSystem && Player->CrouchSystem->IsCrouched())
			return EMovementState::CrouchingMoving; // Landing while moving goes to crouching moving
		
		// Check movement based on horizontal velocity
		float Speed = Player->GetCharacterMovement()->Velocity.Size2D();
		if (Speed > 10.0f)
		{
			if (Player->SprintSystem && Player->SprintSystem->IsSprinting())
				return EMovementState::Sprinting;
			else
				return EMovementState::Walking;
		}
		else
		{
			// No more movement - go to idle
			return EMovementState::Idle;
		}
	}

	return EMovementState::None;
}

bool ULandingMovingMovementState::CanTransitionTo_Implementation(EMovementState NewState) const
{
	// Landing while moving can transition to movement states
	switch (NewState)
	{
	case EMovementState::Walking:
	case EMovementState::Sprinting:
	case EMovementState::CrouchingMoving:
		// Primary transitions after landing while moving
		return true;
	case EMovementState::Idle:
	case EMovementState::CrouchingIdle:
		// Can transition to stationary states
		return true;
	case EMovementState::Dodging:
		// Can dodge during landing
		return true;
	case EMovementState::Jumping:
	case EMovementState::Falling:
		// Can re-enter air states
		return true;
	default:
		return Super::CanTransitionTo_Implementation(NewState);
	}
}