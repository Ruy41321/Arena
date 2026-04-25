// Copyright (c) 2025 Luigi Pennisi. All rights reserved.

#include "Player/PlayerAnimation/MKHPlayerAnimInstance.h"
#include "Player/MovementStateMachine/MovementStateMachine.h"
#include "Player/Components/Crouch/CrouchSystemComponent.h"
#include "Player/MKHPlayerCharacter.h"
#include "Utils/Utils.h"
#include "GameFramework/CharacterMovementComponent.h"

void UMKHPlayerAnimInstance::NativeUninitializeAnimation()
{
	// Unsubscribe from movement state changes
	UnsubscribeFromMovementStateChanges();

	Super::NativeUninitializeAnimation();
}

void UMKHPlayerAnimInstance::NativeBeginPlay()
{
	Super::NativeBeginPlay();
	MKHPlayerCharacter = dynamic_cast<AMKHPlayerCharacter *>(TryGetPawnOwner());
	Speed = 0.0f;
	CurrentMovementState = EMovementStateValue::None;
	PreviousMovementState = EMovementStateValue::None;
	CrouchingTransitionTarget = 0.0f;
	
	// Subscribe to movement state changes
	SubscribeToMovementStateChanges();
}

void UMKHPlayerAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	//if (!MKHPlayerCharacter)
	//	return;
	//	
	// Only smooth interpolation for crouching animations - no more velocity polling!
	// CrouchingTransitionTarget is updated in OnMovementStateChanged when needed
	//if (FMath::Abs(CrouchingTransitionTarget - CrouchingTransitionTime) > 0.1f)
	//{
	//	FUtils::HandleGenericInterpolation(CrouchingTransitionTime, CrouchingTransitionTarget, CrouchingRate, DeltaSeconds, 0.1f);
	//}
}

void UMKHPlayerAnimInstance::OnMovementStateChanged(EMovementStateValue OldState, EMovementStateValue NewState)
{
	// Update movement state properties when notified of changes
	PreviousMovementState = OldState;
	CurrentMovementState = NewState;
	
	// Update crouching transition target based on current state
	if (MKHPlayerCharacter && MKHPlayerCharacter->CrouchSystem)
	{
		CrouchingTransitionTarget = MKHPlayerCharacter->CrouchSystem->IsCrouched() ? 100.0f : 0.0f;
		CrouchingTransitionTime = CrouchingTransitionTarget; // Snap immediately to target for instant transitions
	}
	
	// Update Speed based on current movement state and MaxWalkSpeed for Blend Space locomotion
	if (MKHPlayerCharacter && MKHPlayerCharacter->GetCharacterMovement())
	{
		Speed = MKHPlayerCharacter->GetCharacterMovement()->MaxWalkSpeed;
		if (NewState == EMovementStateValue::Idle || NewState == EMovementStateValue::CrouchingIdle)
		{
			Speed = 0.0f;
		}
	}
}

void UMKHPlayerAnimInstance::SubscribeToMovementStateChanges()
{
	if (!MKHPlayerCharacter || !MKHPlayerCharacter->GetMovementStateMachine() || bIsSubscribedToStateChanges)
		return;
	
	MKHPlayerCharacter->GetMovementStateMachine()->OnStateChanged.AddDynamic(this, &UMKHPlayerAnimInstance::OnMovementStateChanged);
	bIsSubscribedToStateChanges = true;
	
	CurrentMovementState = MKHPlayerCharacter->GetMovementStateMachine()->GetCurrentState();
	PreviousMovementState = MKHPlayerCharacter->GetMovementStateMachine()->GetPreviousState();
}

void UMKHPlayerAnimInstance::UnsubscribeFromMovementStateChanges()
{
	if (!bIsSubscribedToStateChanges || !MKHPlayerCharacter || !MKHPlayerCharacter->GetMovementStateMachine())
		return;
	
	MKHPlayerCharacter->GetMovementStateMachine()->OnStateChanged.RemoveDynamic(this, &UMKHPlayerAnimInstance::OnMovementStateChanged);
	bIsSubscribedToStateChanges = false;
}

bool UMKHPlayerAnimInstance::IsMovementStateDataValid() const
{
	return CurrentMovementState != EMovementStateValue::None && 
		   MKHPlayerCharacter != nullptr && 
		   MKHPlayerCharacter->GetMovementStateMachine() != nullptr &&
		   bIsSubscribedToStateChanges;
}

FString UMKHPlayerAnimInstance::GetCurrentMovementStateString() const
{
	return UMovementStateTypes::MovementStateToString(CurrentMovementState);
}

FString UMKHPlayerAnimInstance::GetPreviousMovementStateString() const
{
	return UMovementStateTypes::MovementStateToString(PreviousMovementState);
}