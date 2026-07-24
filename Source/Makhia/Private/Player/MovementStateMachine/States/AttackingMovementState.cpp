// Copyright (c) 2025 Luigi Pennisi. All rights reserved.

#include "Player/MovementStateMachine/States/AttackingMovementState.h"
#include "Player/MKHPlayerCharacter.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/MKHGameplayTags.h"
#include "Player/Components/Blocking/BlockingSystemComponent.h"

UAttackingMovementState::UAttackingMovementState()
{
}

EMovementStateValue UAttackingMovementState::GetDesiredTransition_Implementation() const
{
	AMKHPlayerCharacter* Player = GetPlayerCharacter();
	if (!Player)
	{
		return EMovementStateValue::None;
	}

	UAbilitySystemComponent* ASC = Player->GetAbilitySystemComponent();
	if (ASC)
	{
		// If there are no priority tags and the player is blocking, allow transition to block state
		bool bHasPriority = ASC->HasMatchingGameplayTag(MKHGameplayTags::Ability::Priority1) ||
							ASC->HasMatchingGameplayTag(MKHGameplayTags::Ability::Priority2);

		if (!bHasPriority && Player->BlockingSystem->IsBlocking())
		{
			return EMovementStateValue::Blocking;
		}

		// Otherwise check the normal case: when the Attacking tag is no longer present
		if (!ASC->HasMatchingGameplayTag(MKHGameplayTags::Ability::Attacking))
		{
			if (Player->BlockingSystem->IsBlocking())
			{
				return EMovementStateValue::Blocking;
			}
			// State exits and returns to idle when the tag disappears
			return EMovementStateValue::Idle;
		}
	}

	return EMovementStateValue::None;
}

bool UAttackingMovementState::CanTransitionTo_Implementation(EMovementStateValue NewState) const
{
	AMKHPlayerCharacter* Player = GetPlayerCharacter();
	if (Player)
	{
		if (UAbilitySystemComponent* ASC = Player->GetAbilitySystemComponent())
		{
			if (ASC->HasMatchingGameplayTag(MKHGameplayTags::Ability::Attacking))
			{
				// Allow transition to block state while attacking as long as there are no priority tags
				if (NewState == EMovementStateValue::Blocking)
				{
					bool bHasPriority = ASC->HasMatchingGameplayTag(MKHGameplayTags::Ability::Priority1) ||
										ASC->HasMatchingGameplayTag(MKHGameplayTags::Ability::Priority2);
					if (bHasPriority)
					{
						return false;
					}
				}
				else
				{
					// Still have the Attacking tag and not blocking, deny the transition
					return false;
				}
			}
		}
	}
	return Super::CanTransitionTo_Implementation(NewState);
}
