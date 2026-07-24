// Copyright (c) 2025 Luigi Pennisi. All rights reserved.

#include "Player/MovementStateMachine/States/LandingMovingMovementState.h"
#include "Player/Components/Jump/JumpSystemComponent.h"
#include "Player/Components/Sprint/SprintSystemComponent.h"
#include "Player/Components/Crouch/CrouchSystemComponent.h"
#include "Player/MKHPlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Player/Components/Blocking/BlockingSystemComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/MKHGameplayTags.h"

ULandingMovingMovementState::ULandingMovingMovementState()
{
}

EMovementStateValue ULandingMovingMovementState::GetDesiredTransition_Implementation() const
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

	// Check if landing state is finished
	if (!Player->JumpSystem || !Player->JumpSystem->IsLanding())
	{
		// Landing finished - transition based on current conditions
		
		// Enter block state on state ending if the player was willing to block during this state
		if (Player->BlockingSystem && Player->BlockingSystem->IsBlocking())
		{
			return EMovementStateValue::Blocking;
		}
		
		if (Player->CrouchSystem && Player->CrouchSystem->IsCrouched())
			return EMovementStateValue::CrouchingMoving; // Landing while moving goes to crouching moving
		
		// Check movement based on horizontal velocity
		float Speed = Player->GetCharacterMovement()->Velocity.Size2D();
		if (Speed > 10.0f)
		{
			if (Player->SprintSystem && Player->SprintSystem->IsSprinting())
				return EMovementStateValue::Sprinting;
			else
				return EMovementStateValue::Walking;
		}
		else
		{
			// No more movement - go to idle
			return EMovementStateValue::Idle;
		}
	}

	return EMovementStateValue::None;
}

bool ULandingMovingMovementState::CanTransitionTo_Implementation(EMovementStateValue NewState) const
{
	return Super::CanTransitionTo_Implementation(NewState);
}