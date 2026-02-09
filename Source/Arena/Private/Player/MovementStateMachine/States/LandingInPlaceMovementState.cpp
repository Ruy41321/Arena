// Copyright (c) 2025 Luigi Pennisi. All rights reserved.

#include "Player/MovementStateMachine/States/LandingInPlaceMovementState.h"
#include "Player/Components/Jump/JumpSystemComponent.h"
#include "Player/Components/Crouch/CrouchSystemComponent.h"
#include "Player/PlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

ULandingInPlaceMovementState::ULandingInPlaceMovementState()
{
}

EMovementState ULandingInPlaceMovementState::GetDesiredTransition_Implementation() const
{
	APlayerCharacter* Player = GetPlayerCharacter();
	if (!Player || !Player->GetCharacterMovement())
		return EMovementState::None;

	// Check if landing state is finished
	if (!Player->JumpSystem || !Player->JumpSystem->IsLanding())
	{
		// Landing finished - transition based on current conditions
		if (Player->CrouchSystem && Player->CrouchSystem->IsCrouched())
			return EMovementState::CrouchingIdle; // Landing in place goes to crouching idle
		
		// Since this is landing in place, we expect to go to Idle
		return EMovementState::Idle;
	}

	return EMovementState::None;
}

bool ULandingInPlaceMovementState::CanTransitionTo_Implementation(EMovementState NewState) const
{
	// Landing in place can transition to ground states
	switch (NewState)
	{
	case EMovementState::Idle:
	case EMovementState::CrouchingIdle:
		// Primary transitions after landing in place
		return true;
	case EMovementState::Walking:
	case EMovementState::Sprinting:
	case EMovementState::CrouchingMoving:
		// Can transition to movement if input starts during landing
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