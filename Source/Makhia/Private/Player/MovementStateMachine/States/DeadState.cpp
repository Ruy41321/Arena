// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/MovementStateMachine/States/DeadState.h"

#include "Components/CapsuleComponent.h"
#include "Engine/CollisionProfile.h"
#include "Player/MKHPlayerCharacter.h"
#include "Player/PlayerController/MKHPlayerController.h"

UDeadState::UDeadState()
{
}

void UDeadState::EnterState(EMovementStateValue PreviousState)
{
	Super::EnterState(PreviousState);


	if (MKHPlayerCharacter && MKHPlayerCharacter->GetCapsuleComponent())
	{
		//Disable player collision
		MKHPlayerCharacter->GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		MKHPlayerCharacter->GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);
		MKHPlayerCharacter->GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Block);
	}

	// Strip the gameplay bindings so a dead character cannot move, look around or trigger actions.
	SetOwnerGameplayInputEnabled(false);
}

void UDeadState::ExitState(EMovementStateValue NextState)
{
	Super::ExitState(NextState);

	// Re-enable the capsule collision disabled by EnterState. Exiting the Dead state only happens
	// through a forced transition (e.g. a revive between duel rounds), so restoring the standard
	// Pawn collision profile here brings the character fully back to a collidable, playable state.
	if (IsValid(MKHPlayerCharacter) && MKHPlayerCharacter->GetCapsuleComponent())
	{
		MKHPlayerCharacter->GetCapsuleComponent()->SetCollisionProfileName(UCollisionProfile::Pawn_ProfileName);
	}

	// Give the bindings back to the revived character.
	SetOwnerGameplayInputEnabled(true);
}

bool UDeadState::CanTransitionTo_Implementation(EMovementStateValue NewState) const
{
	return false;
}

void UDeadState::SetOwnerGameplayInputEnabled(const bool bEnabled) const
{
	if (!IsValid(MKHPlayerCharacter) || !MKHPlayerCharacter->IsLocallyControlled())
	{
		return;
	}

	if (AMKHPlayerController* MKHPlayerController = Cast<AMKHPlayerController>(MKHPlayerCharacter->GetController()))
	{
		MKHPlayerController->SetGameplayInputEnabled(bEnabled);
	}
}
