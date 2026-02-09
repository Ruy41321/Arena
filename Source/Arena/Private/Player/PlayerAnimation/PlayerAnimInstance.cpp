// Copyright (c) 2025 Luigi Pennisi. All rights reserved.

#include "Player/PlayerAnimation/PlayerAnimInstance.h"
#include "Player/MovementStateMachine/MovementStateMachine.h"
#include "Player/Components/Crouch/CrouchSystemComponent.h"
#include "Player/PlayerCharacter.h"
#include "Utils/Utils.h"
#include "GameFramework/CharacterMovementComponent.h"

void UPlayerAnimInstance::NativeUninitializeAnimation()
{
	// Unsubscribe from movement state changes
	UnsubscribeFromMovementStateChanges();

	Super::NativeUninitializeAnimation();
}

void UPlayerAnimInstance::NativeBeginPlay()
{
	Super::NativeBeginPlay();
	PlayerCharacter = dynamic_cast<APlayerCharacter *>(TryGetPawnOwner());
	Speed = 0.0f;
	CurrentMovementState = EMovementState::None;
	PreviousMovementState = EMovementState::None;
	CrouchingTransitionTarget = 0.0f;
	
	// Subscribe to movement state changes
	SubscribeToMovementStateChanges();
}

void UPlayerAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	//if (!PlayerCharacter)
	//	return;
	//	
	// Only smooth interpolation for crouching animations - no more velocity polling!
	// CrouchingTransitionTarget is updated in OnMovementStateChanged when needed
	//if (FMath::Abs(CrouchingTransitionTarget - CrouchingTransitionTime) > 0.1f)
	//{
	//	FUtils::HandleGenericInterpolation(CrouchingTransitionTime, CrouchingTransitionTarget, CrouchingRate, DeltaSeconds, 0.1f);
	//}
}

void UPlayerAnimInstance::OnMovementStateChanged(EMovementState OldState, EMovementState NewState)
{
	// Update movement state properties when notified of changes
	PreviousMovementState = OldState;
	CurrentMovementState = NewState;
	
	// Update crouching transition target based on current state
	if (PlayerCharacter && PlayerCharacter->CrouchSystem)
	{
		CrouchingTransitionTarget = PlayerCharacter->CrouchSystem->IsCrouched() ? 100.0f : 0.0f;
		CrouchingTransitionTime = CrouchingTransitionTarget; // Snap immediately to target for instant transitions
	}
	
	// Update Speed based on current movement state and MaxWalkSpeed for Blend Space locomotion
	if (PlayerCharacter && PlayerCharacter->GetCharacterMovement())
	{
		Speed = PlayerCharacter->GetCharacterMovement()->MaxWalkSpeed;
		if (NewState == EMovementState::Idle || NewState == EMovementState::CrouchingIdle)
		{
			Speed = 0.0f;
		}
	}
}

void UPlayerAnimInstance::SubscribeToMovementStateChanges()
{
	if (!PlayerCharacter || !PlayerCharacter->GetMovementStateMachine() || bIsSubscribedToStateChanges)
		return;
	
	PlayerCharacter->GetMovementStateMachine()->OnStateChanged.AddDynamic(this, &UPlayerAnimInstance::OnMovementStateChanged);
	bIsSubscribedToStateChanges = true;
	
	CurrentMovementState = PlayerCharacter->GetMovementStateMachine()->GetCurrentState();
	PreviousMovementState = PlayerCharacter->GetMovementStateMachine()->GetPreviousState();
}

void UPlayerAnimInstance::UnsubscribeFromMovementStateChanges()
{
	if (!bIsSubscribedToStateChanges || !PlayerCharacter || !PlayerCharacter->GetMovementStateMachine())
		return;
	
	PlayerCharacter->GetMovementStateMachine()->OnStateChanged.RemoveDynamic(this, &UPlayerAnimInstance::OnMovementStateChanged);
	bIsSubscribedToStateChanges = false;
}

bool UPlayerAnimInstance::IsMovementStateDataValid() const
{
	return CurrentMovementState != EMovementState::None && 
		   PlayerCharacter != nullptr && 
		   PlayerCharacter->GetMovementStateMachine() != nullptr &&
		   bIsSubscribedToStateChanges;
}

FString UPlayerAnimInstance::GetCurrentMovementStateString() const
{
	return UMovementStateTypes::MovementStateToString(CurrentMovementState);
}

FString UPlayerAnimInstance::GetPreviousMovementStateString() const
{
	return UMovementStateTypes::MovementStateToString(PreviousMovementState);
}