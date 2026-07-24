// Copyright (c) 2025 Luigi Pennisi. All rights reserved.

#include "Player/PlayerAnimation/MKHPlayerAnimInstance.h"

#include "KismetAnimationLibrary.h"
#include "Player/MovementStateMachine/MovementStateMachine.h"
#include "Player/Components/Crouch/CrouchSystemComponent.h"
#include "Player/MKHPlayerCharacter.h"

void UMKHPlayerAnimInstance::NativeUninitializeAnimation()
{
	// Unsubscribe from movement state changes
	UnsubscribeFromMovementStateChanges();

	Super::NativeUninitializeAnimation();
}

void UMKHPlayerAnimInstance::NativeBeginPlay()
{
	Super::NativeBeginPlay();
	MKHPlayerCharacter = Cast<AMKHPlayerCharacter>(TryGetPawnOwner());
	CurrentMovementState = EMovementStateValue::None;
	PreviousMovementState = EMovementStateValue::None;
	CrouchingTransitionTarget = 0.0f;
	
	SubscribeToMovementStateChanges();
}

void UMKHPlayerAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	
	if (!IsValid(MKHPlayerCharacter))
		return;
	
	CurrentSpeed = MKHPlayerCharacter->GetVelocity().Size();
	Direction = UKismetAnimationLibrary::CalculateDirection(MKHPlayerCharacter->GetVelocity(), MKHPlayerCharacter->GetActorRotation());
	
	bIsHoldingWeapon = MKHPlayerCharacter->bIsHoldingWeapon;
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