// Copyright (c) 2025 Luigi Pennisi. All rights reserved.

#include "Player/MovementStateMachine/States/LandingInPlaceMovementState.h"
#include "Player/Components/Jump/JumpSystemComponent.h"
#include "Player/Components/Crouch/CrouchSystemComponent.h"
#include "Player/MKHPlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Player/Components/Blocking/BlockingSystemComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/MKHGameplayTags.h"

ULandingInPlaceMovementState::ULandingInPlaceMovementState()
{
}

EMovementStateValue ULandingInPlaceMovementState::GetDesiredTransition_Implementation() const
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
			return EMovementStateValue::CrouchingIdle; // Landing in place goes to crouching idle
		
		// Since this is landing in place, we expect to go to Idle
		return EMovementStateValue::Idle;
	}

	return EMovementStateValue::None;
}

bool ULandingInPlaceMovementState::CanTransitionTo_Implementation(EMovementStateValue NewState) const
{
	return Super::CanTransitionTo_Implementation(NewState);
}