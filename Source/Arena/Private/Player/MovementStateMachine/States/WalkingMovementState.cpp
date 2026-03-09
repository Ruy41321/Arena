// Copyright (c) 2025 Luigi Pennisi. All rights reserved.

#include "Player/MovementStateMachine/States/WalkingMovementState.h"
#include "Player/Components/Dodge/DodgeSystemComponent.h"
#include "Player/Components/Crouch/CrouchSystemComponent.h"
#include "Player/Components/Sprint/SprintSystemComponent.h"
#include "Player/PlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

UWalkingMovementState::UWalkingMovementState()
{
}

EMovementStateValue UWalkingMovementState::GetDesiredTransition_Implementation() const
{
	APlayerCharacter* Player = GetPlayerCharacter();
	if (!Player || !Player->GetCharacterMovement())
		return EMovementStateValue::None;

	// Check for falling first
	if (Player->GetCharacterMovement()->IsFalling())
	{
		FVector Velocity = Player->GetCharacterMovement()->Velocity;
		if (Velocity.Z > 0.0f)
			return EMovementStateValue::Jumping;
		else
			return EMovementStateValue::Falling;
	}

	// Check for dodging
	if (Player->DodgeSystem && Player->DodgeSystem->IsDodging())
		return EMovementStateValue::Dodging;

	// Check for crouching - decide between idle and moving based on velocity
	if (Player->CrouchSystem && Player->CrouchSystem->IsCrouched())
	{
		float HorizontalSpeed = Player->GetCharacterMovement()->Velocity.Size2D();
		if (HorizontalSpeed > 0.1f)
			return EMovementStateValue::CrouchingMoving;
		else
			return EMovementStateValue::CrouchingIdle;
	}

	// Check movement speed
	float Speed = Player->GetCharacterMovement()->Velocity.Size2D();
	
	// If no movement, go to idle
	if (Speed <= 10.0f)
		return EMovementStateValue::Idle;

	// Check if sprinting
	if (Player->SprintSystem && Player->SprintSystem->IsSprinting())
		return EMovementStateValue::Sprinting;

	// Stay in walking state
	return EMovementStateValue::None;
}
