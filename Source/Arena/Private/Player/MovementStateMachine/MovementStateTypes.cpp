// Copyright (c) 2025 Luigi Pennisi. All rights reserved.

#include "Player/MovementStateMachine/MovementStateTypes.h"

FString UMovementStateTypes::MovementStateToString(EMovementState State)
{
	switch (State)
	{
	case EMovementState::None:
		return TEXT("None");
	case EMovementState::Idle:
		return TEXT("Idle");
	case EMovementState::Walking:
		return TEXT("Walking");
	case EMovementState::Sprinting:
		return TEXT("Sprinting");
	case EMovementState::CrouchingIdle:
		return TEXT("Crouching Idle");
	case EMovementState::CrouchingMoving:
		return TEXT("Crouching Moving");
	case EMovementState::Jumping:
		return TEXT("Jumping");
	case EMovementState::Falling:
		return TEXT("Falling");
	case EMovementState::LandingInPlace:
		return TEXT("Landing In Place");
	case EMovementState::LandingMoving:
		return TEXT("Landing Moving");
	case EMovementState::Dodging:
		return TEXT("Dodging");
	default:
		return TEXT("Unknown");
	}
}

bool UMovementStateTypes::IsGroundedState(EMovementState State)
{
	switch (State)
	{
	case EMovementState::Idle:
	case EMovementState::Walking:
	case EMovementState::Sprinting:
	case EMovementState::CrouchingIdle:
	case EMovementState::CrouchingMoving:
	case EMovementState::LandingInPlace:
	case EMovementState::LandingMoving:
	case EMovementState::Dodging:
		return true;
	default:
		return false;
	}
}

bool UMovementStateTypes::IsAirborneState(EMovementState State)
{
	switch (State)
	{
	case EMovementState::Jumping:
	case EMovementState::Falling:
		return true;
	default:
		return false;
	}
}

bool UMovementStateTypes::CanReceiveMovementInput(EMovementState State)
{
	switch (State)
	{
	case EMovementState::Idle:
	case EMovementState::Walking:
	case EMovementState::Sprinting:
	case EMovementState::CrouchingIdle:
	case EMovementState::CrouchingMoving:
	case EMovementState::Jumping:
	case EMovementState::Falling:
		return true;
	case EMovementState::LandingInPlace:
	case EMovementState::LandingMoving:
	case EMovementState::Dodging:
		return false;
	default:
		return false;
	}
}
