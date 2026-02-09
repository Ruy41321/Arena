// Copyright (c) 2025 Luigi Pennisi. All rights reserved.

#include "Player/MovementStateMachine/MovementState.h"
#include "Player/MovementStateMachine/MovementStateMachine.h"
#include "Player/PlayerCharacter.h"
// Include component headers for speed getters
#include "Player/Components/Dodge/DodgeSystemComponent.h"
#include "Player/Components/Crouch/CrouchSystemComponent.h"
#include "Player/Components/Sprint/SprintSystemComponent.h"
#include "Player/Components/BasicMovement/BasicMovementComponent.h"

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

bool UMovementState::CanTransitionTo_Implementation(EMovementStateValue NewState) const
{
	// Default implementation allows all transitions
	return true;
}

EMovementStateValue UMovementState::GetDesiredTransition_Implementation() const
{
	// Default implementation: no automatic transitions
	return EMovementStateValue::None;
}

void UMovementState::EnterState(EMovementStateValue PreviousState)
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

void UMovementState::ExitState(EMovementStateValue NextState)
{
	// Call Blueprint event
	OnExitState(NextState);
}

void UMovementState::SetStateSpeed()
{
	APlayerCharacter* Player = GetPlayerCharacter();
	if (!Player)
		return;

	EMovementStateValue StateType = GetStateType();
	float Speed = GetSpeedForState(StateType);
	
	Player->SetMaxWalkSpeed(Speed);
}

float UMovementState::GetSpeedForState(EMovementStateValue StateType) const
{
	APlayerCharacter* Player = GetPlayerCharacter();
	if (!Player)
		return 300.0f; // Default fallback speed

	switch (StateType)
	{
	case EMovementStateValue::Dodging:
		// Get dodge speed from DodgeSystem
		if (Player->DodgeSystem)
			return Player->DodgeSystem->GetDodgeSpeed();
		return 600.0f; // Fallback dodge speed
		
	case EMovementStateValue::CrouchingIdle:
	case EMovementStateValue::CrouchingMoving:
		// Get crouch speed from CrouchSystem
		if (Player->CrouchSystem)
			return Player->CrouchSystem->GetCrouchSpeed();
		return 150.0f; // Fallback crouch speed
		
	case EMovementStateValue::Sprinting:
		// Get run speed from SprintSystem
		if (Player->SprintSystem)
			return Player->SprintSystem->GetRunSpeed();
		return 600.0f; // Fallback sprint speed
		
	case EMovementStateValue::Idle:
	case EMovementStateValue::Walking:
	case EMovementStateValue::Jumping:
	case EMovementStateValue::Falling:
	case EMovementStateValue::LandingInPlace:
	case EMovementStateValue::LandingMoving:
	default:
		// Get walk speed from BasicMovementSystem
		if (Player->BasicMovementSystem)
			return Player->BasicMovementSystem->GetWalkSpeed();
		return 300.0f; // Fallback walk speed
	}
}
