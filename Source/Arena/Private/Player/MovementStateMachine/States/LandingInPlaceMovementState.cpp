// Copyright (c) 2025 Luigi Pennisi. All rights reserved.

#include "Player/MovementStateMachine/States/LandingInPlaceMovementState.h"
#include "Player/Components/Jump/JumpSystemComponent.h"
#include "Player/Components/Crouch/CrouchSystemComponent.h"
#include "Player/PlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

ULandingInPlaceMovementState::ULandingInPlaceMovementState()
{
}

EMovementStateValue ULandingInPlaceMovementState::GetDesiredTransition_Implementation() const
{
	APlayerCharacter* Player = GetPlayerCharacter();
	if (!Player || !Player->GetCharacterMovement())
		return EMovementStateValue::None;

	// Check if landing state is finished
	if (!Player->JumpSystem || !Player->JumpSystem->IsLanding())
	{
		// Landing finished - transition based on current conditions
		if (Player->CrouchSystem && Player->CrouchSystem->IsCrouched())
			return EMovementStateValue::CrouchingIdle; // Landing in place goes to crouching idle
		
		// Since this is landing in place, we expect to go to Idle
		return EMovementStateValue::Idle;
	}

	return EMovementStateValue::None;
}

bool ULandingInPlaceMovementState::CanTransitionTo_Implementation(EMovementStateValue NewState) const
{
	// Landing in place can transition to ground states
	switch (NewState)
	{
	case EMovementStateValue::Idle:
	case EMovementStateValue::CrouchingIdle:
		// Primary transitions after landing in place
		return true;
	case EMovementStateValue::Walking:
	case EMovementStateValue::Sprinting:
	case EMovementStateValue::CrouchingMoving:
		// Can transition to movement if input starts during landing
		return true;
	case EMovementStateValue::Dodging:
		// Can dodge during landing
		return true;
	case EMovementStateValue::Jumping:
	case EMovementStateValue::Falling:
		// Can re-enter air states
		return true;
	default:
		return Super::CanTransitionTo_Implementation(NewState);
	}
}