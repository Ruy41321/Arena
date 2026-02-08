// Copyright (c) 2025 Luigi Pennisi. All rights reserved.

#include "MovementState.h"
#include "MovementStateMachine.h"
#include "../PlayerCharacter.h"
// Include component headers for speed getters
#include "../../Components/Dodge/DodgeSystemComponent.h"
#include "../../Components/Crouch/CrouchSystemComponent.h"
#include "../../Components/Sprint/SprintSystemComponent.h"
#include "../../Components/BasicMovement/BasicMovementComponent.h"

UMovementState::UMovementState()
{
	StateMachine = nullptr;
	PlayerCharacter = nullptr;
}

void UMovementState::Initialize(UMovementStateMachine* InStateMachine, APlayerCharacter* InPlayerCharacter)
{
	StateMachine = InStateMachine;
	PlayerCharacter = InPlayerCharacter;
}

bool UMovementState::CanTransitionTo_Implementation(EMovementState NewState) const
{
	// Default implementation allows all transitions
	return true;
}

EMovementState UMovementState::GetDesiredTransition_Implementation() const
{
	// Default implementation: no automatic transitions
	return EMovementState::None;
}

void UMovementState::EnterState(EMovementState PreviousState)
{
	// Set the appropriate speed for this state
	SetStateSpeed();
	
	// Call Blueprint event
	OnEnterState(PreviousState);
}

void UMovementState::UpdateState(float DeltaTime)
{
	// Call Blueprint event
	OnUpdateState(DeltaTime);
}

void UMovementState::ExitState(EMovementState NextState)
{
	// Call Blueprint event
	OnExitState(NextState);
}

void UMovementState::SetStateSpeed()
{
	APlayerCharacter* Player = GetPlayerCharacter();
	if (!Player)
		return;

	EMovementState StateType = GetStateType();
	float Speed = GetSpeedForState(StateType);
	
	Player->SetMaxWalkSpeed(Speed);
}

float UMovementState::GetSpeedForState(EMovementState StateType) const
{
	APlayerCharacter* Player = GetPlayerCharacter();
	if (!Player)
		return 300.0f; // Default fallback speed

	switch (StateType)
	{
	case EMovementState::Dodging:
		// Get dodge speed from DodgeSystem
		if (Player->DodgeSystem)
			return Player->DodgeSystem->GetDodgeSpeed();
		return 600.0f; // Fallback dodge speed
		
	case EMovementState::CrouchingIdle:
	case EMovementState::CrouchingMoving:
		// Get crouch speed from CrouchSystem
		if (Player->CrouchSystem)
			return Player->CrouchSystem->GetCrouchSpeed();
		return 150.0f; // Fallback crouch speed
		
	case EMovementState::Sprinting:
		// Get run speed from SprintSystem
		if (Player->SprintSystem)
			return Player->SprintSystem->GetRunSpeed();
		return 600.0f; // Fallback sprint speed
		
	case EMovementState::Idle:
	case EMovementState::Walking:
	case EMovementState::Jumping:
	case EMovementState::Falling:
	case EMovementState::LandingInPlace:
	case EMovementState::LandingMoving:
	default:
		// Get walk speed from BasicMovementSystem
		if (Player->BasicMovementSystem)
			return Player->BasicMovementSystem->GetWalkSpeed();
		return 300.0f; // Fallback walk speed
	}
}
