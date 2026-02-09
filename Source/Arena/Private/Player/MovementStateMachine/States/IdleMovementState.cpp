// Copyright (c) 2025 Luigi Pennisi. All rights reserved.

#include "Player/MovementStateMachine/States/IdleMovementState.h"
#include "Player/Components/Dodge/DodgeSystemComponent.h"
#include "Player/Components/Sprint/SprintSystemComponent.h"
#include "Player/Components/Crouch/CrouchSystemComponent.h"
#include "Player/PlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

UIdleMovementState::UIdleMovementState()
{
}

EMovementStateValue UIdleMovementState::GetDesiredTransition_Implementation() const
{
	APlayerCharacter* Player = GetPlayerCharacter();
	if (!Player || !Player->GetCharacterMovement())
		return EMovementStateValue::None;

	// Check for falling
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

	// Check for movement
	float Speed = Player->GetCharacterMovement()->Velocity.Size2D();
	if (Speed > 10.0f) // Small threshold to avoid jitter
	{
		// Check if sprinting
		if (Player->SprintSystem && Player->SprintSystem->IsSprinting())
			return EMovementStateValue::Sprinting;
		else
			return EMovementStateValue::Walking;
	}

	return EMovementStateValue::None;
}

void UIdleMovementState::EnterState(EMovementStateValue PreviousState)
{
	Super::EnterState(PreviousState);
	// Additional logic for entering idle state can be added here
}
