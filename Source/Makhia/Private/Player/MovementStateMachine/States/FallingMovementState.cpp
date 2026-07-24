// Copyright (c) 2025 Luigi Pennisi. All rights reserved.

#include "Player/MovementStateMachine/States/FallingMovementState.h"
#include "Player/MKHPlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Player/Components/Blocking/BlockingSystemComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/MKHGameplayTags.h"

UFallingMovementState::UFallingMovementState()
{
}

EMovementStateValue UFallingMovementState::GetDesiredTransition_Implementation() const
{
	AMKHPlayerCharacter* Player = GetPlayerCharacter();
	if (!Player || !Player->GetCharacterMovement())
		return EMovementStateValue::None;

	// Check for attacking
	if (UAbilitySystemComponent* ASC = Player->GetAbilitySystemComponent())
	{
		if (ASC->HasMatchingGameplayTag(MKHGameplayTags::Ability::Attacking))
		{
			return EMovementStateValue::Attacking;
		}
	}

	// Check if we've landed
	if (!Player->GetCharacterMovement()->IsFalling())
	{
		// Enter block state on state ending if the player was willing to block during this state
		if (Player->BlockingSystem && Player->BlockingSystem->IsBlocking())
		{
			return EMovementStateValue::Blocking;
		}
		
		// Decide between LandingInPlace and LandingMoving based on horizontal velocity
		float HorizontalSpeed = Player->GetCharacterMovement()->Velocity.Size2D();
		if (HorizontalSpeed > 0.1f) // Small threshold to avoid floating-point precision issues
			return EMovementStateValue::LandingMoving;
		else
			return EMovementStateValue::LandingInPlace;
	}

	return EMovementStateValue::None;
}

bool UFallingMovementState::CanTransitionTo_Implementation(EMovementStateValue NewState) const
{
	return Super::CanTransitionTo_Implementation(NewState);
}
