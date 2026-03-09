// Copyright (c) 2025 Luigi Pennisi. All rights reserved.

#include "Player/MovementStateMachine/States/SprintingMovementState.h"
#include "Player/Components/Dodge/DodgeSystemComponent.h"
#include "Player/Components/Crouch/CrouchSystemComponent.h"
#include "Player/Components/Sprint/SprintSystemComponent.h"
#include "Player/PlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

USprintingMovementState::USprintingMovementState()
{
}

EMovementStateValue USprintingMovementState::GetDesiredTransition_Implementation() const
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

	// Check if still sprinting
	if (Player->SprintSystem)
	{
		// Check movement speed to determine if walking or idle
		float Speed = Player->GetCharacterMovement()->Velocity.Size2D();
		if (Speed == 0.0f)
			return EMovementStateValue::Idle;
		else if (!Player->SprintSystem->IsSprinting())
			return EMovementStateValue::Walking;
	}

	// Stay in sprinting state
	return EMovementStateValue::None;
}
