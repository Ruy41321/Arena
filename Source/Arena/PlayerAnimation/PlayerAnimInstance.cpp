// Copyright (c) 2025 Luigi Pennisi. All rights reserved.

#include "PlayerAnimInstance.h"
#include "GameFramework/CharacterMovementComponent.h"

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

void UPlayerAnimInstance::NativeUninitializeAnimation()
{
	// Unsubscribe from movement state changes
	UnsubscribeFromMovementStateChanges();
	
	Super::NativeUninitializeAnimation();
}

void UPlayerAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	if (!PlayerCharacter)
		return;
		
	// Only smooth interpolation for crouching animations - no more velocity polling!
	// CrouchingTransitionTarget is updated in OnMovementStateChanged when needed
	if (FMath::Abs(CrouchingTransitionTarget - CrouchingTransitionTime) > 0.1f)
	{
		FUtils::HandleGenericInterpolation(CrouchingTransitionTime, CrouchingTransitionTarget, CrouchingRate, DeltaSeconds, 0.1f);
	}
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
	}
	
	// Update Speed based on current movement state and MaxWalkSpeed for Blend Space locomotion
	if (PlayerCharacter && PlayerCharacter->GetCharacterMovement())
	{
		Speed = PlayerCharacter->GetCharacterMovement()->MaxWalkSpeed;
		if (NewState == EMovementState::Idle || NewState == EMovementState::CrouchingIdle)
		{
			Speed *= 0.0f;
		}
	}
	
	UE_LOG(LogTemp, Verbose, TEXT("PlayerAnimInstance: Movement state changed from %s to %s, Speed set to %f"), 
		*UMovementStateTypes::MovementStateToString(OldState),
		*UMovementStateTypes::MovementStateToString(NewState),
		Speed);
}

void UPlayerAnimInstance::SubscribeToMovementStateChanges()
{
	if (!PlayerCharacter || !PlayerCharacter->MovementStateMachine || bIsSubscribedToStateChanges)
		return;
	
	// Bind to the movement state machine's OnStateChanged delegate
	PlayerCharacter->MovementStateMachine->OnStateChanged.AddDynamic(this, &UPlayerAnimInstance::OnMovementStateChanged);
	bIsSubscribedToStateChanges = true;
	
	// Initialize current state values
	CurrentMovementState = PlayerCharacter->MovementStateMachine->GetCurrentState();
	PreviousMovementState = PlayerCharacter->MovementStateMachine->GetPreviousState();
	
	UE_LOG(LogTemp, Log, TEXT("PlayerAnimInstance: Subscribed to movement state changes. Current state: %s"), 
		*UMovementStateTypes::MovementStateToString(CurrentMovementState));
}

void UPlayerAnimInstance::UnsubscribeFromMovementStateChanges()
{
	if (!bIsSubscribedToStateChanges || !PlayerCharacter || !PlayerCharacter->MovementStateMachine)
		return;
	
	// Unbind from the movement state machine's OnStateChanged delegate
	PlayerCharacter->MovementStateMachine->OnStateChanged.RemoveDynamic(this, &UPlayerAnimInstance::OnMovementStateChanged);
	bIsSubscribedToStateChanges = false;
	
	UE_LOG(LogTemp, Log, TEXT("PlayerAnimInstance: Unsubscribed from movement state changes"));
}

bool UPlayerAnimInstance::IsMovementStateDataValid() const
{
	return CurrentMovementState != EMovementState::None && 
		   PlayerCharacter != nullptr && 
		   PlayerCharacter->MovementStateMachine != nullptr &&
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