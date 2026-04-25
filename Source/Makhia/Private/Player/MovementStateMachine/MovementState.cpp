// Copyright (c) 2025 Luigi Pennisi. All rights reserved.

#include "Player/MovementStateMachine/MovementState.h"
#include "Player/MovementStateMachine/MovementStateMachine.h"
#include "Player/MKHPlayerCharacter.h"
// Include component headers for speed getters
#include "Player/Components/Dodge/DodgeSystemComponent.h"
#include "Player/Components/Crouch/CrouchSystemComponent.h"
#include "Player/Components/Sprint/SprintSystemComponent.h"
#include "Player/Components/BasicMovement/BasicMovementComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/MKHGameplayTags.h"

UMovementState::UMovementState()
{
	StateMachine = nullptr;
	MKHPlayerCharacter = nullptr;
}

void UMovementState::Initialize(UMovementStateMachine* InStateMachine, AMKHPlayerCharacter* InPlayerCharacter)
{
	StateMachine = InStateMachine;
	MKHPlayerCharacter = InPlayerCharacter;
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

	// Add the movement state gameplay tag to the ASC
	if (AMKHPlayerCharacter* Player = GetPlayerCharacter())
	{
		if (UAbilitySystemComponent* ASC = Player->GetAbilitySystemComponent())
		{
			const FGameplayTag StateTag = GetTagForState(GetStateType());
			if (StateTag.IsValid())
			{
				ASC->AddLooseGameplayTag(StateTag);
			}
		}
	}
	
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
	// Remove the movement state gameplay tag from the ASC
	if (AMKHPlayerCharacter* Player = GetPlayerCharacter())
	{
		if (UAbilitySystemComponent* ASC = Player->GetAbilitySystemComponent())
		{
			const FGameplayTag StateTag = GetTagForState(GetStateType());
			if (StateTag.IsValid() && ASC->HasMatchingGameplayTag(StateTag))
			{
				ASC->RemoveLooseGameplayTag(StateTag);
			}
		}
	}

	// Call Blueprint event
	OnExitState(NextState);
}

void UMovementState::SetStateSpeed()
{
	AMKHPlayerCharacter* Player = GetPlayerCharacter();
	if (!Player)
		return;

	EMovementStateValue StateType = GetStateType();
	float Speed = GetSpeedForState(StateType);
	
	Player->SetMaxWalkSpeed(Speed);
}

float UMovementState::GetSpeedForState(EMovementStateValue StateType) const
{
	AMKHPlayerCharacter* Player = GetPlayerCharacter();
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

FGameplayTag UMovementState::GetTagForState(EMovementStateValue StateType)
{
	using namespace MKHGameplayTags::State::Movement;

	switch (StateType)
	{
	case EMovementStateValue::Idle:            return Idle;
	case EMovementStateValue::Walking:         return Walking;
	case EMovementStateValue::Sprinting:       return Sprinting;
	case EMovementStateValue::CrouchingIdle:   return CrouchingIdle;
	case EMovementStateValue::CrouchingMoving: return CrouchingMoving;
	case EMovementStateValue::Jumping:         return Jumping;
	case EMovementStateValue::Falling:         return Falling;
	case EMovementStateValue::LandingInPlace:  return LandingInPlace;
	case EMovementStateValue::LandingMoving:   return LandingMoving;
	case EMovementStateValue::Dodging:         return Dodging;
	default:                                   return FGameplayTag();
	}
}

