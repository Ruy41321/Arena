// Fill out your copyright notice in the Description page of Project Settings.

#include "PlayerAnimInstance.h"
#include "GameFramework/CharacterMovementComponent.h"

void UPlayerAnimInstance::NativeBeginPlay()
{
	Super::NativeBeginPlay();
	PlayerCharacter = dynamic_cast<APlayerCharacter *>(TryGetPawnOwner());
	Speed = 0.0f;
}

void UPlayerAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	if (!PlayerCharacter)
		return;
	Velocity = PlayerCharacter->GetCharacterMovement()->Velocity;
	Speed = Velocity.Size2D();
	IsJumping = PlayerCharacter->GetCharacterMovement()->IsFalling() and Velocity.Z > 0.0f;
	IsFalling = PlayerCharacter->GetCharacterMovement()->IsFalling() and Velocity.Z <= 0.0f;
	IsLanding = PlayerCharacter->IsLanding;
	IsDodging = PlayerCharacter->DodgeSystem->IsDodging();
	CanDodge = PlayerCharacter->DodgeSystem->CanDodge();
	float CrouchingTransitionTarget = PlayerCharacter->IsCrouched ? 100.0f : 0.0f;
	if (CrouchingTransitionTarget != CrouchingTransitionTime)
		FUtils::HandleGenericInterpolation(CrouchingTransitionTime, CrouchingTransitionTarget, CrouchingRate, DeltaSeconds, 0.1f);
}

FString UPlayerAnimInstance::GetCurrentStateMachineName()
{
	// Get the state machine index first
	int32 StateMachineIndex = GetStateMachineIndex(FName("SM_Anim"));

	// Check if the state machine was found (valid index)
	if (StateMachineIndex == INDEX_NONE || StateMachineIndex < 0)
	{
		return FString("State machine 'SM_Anim' not found");
	}

	// Get the current state name
	FName CurrentStateName = GetCurrentStateName(StateMachineIndex);

	// Check if the state name is valid
	if (CurrentStateName == NAME_None || !CurrentStateName.IsValid())
	{
		return FString("No valid state found");
	}

	// Return the state name as string
	return CurrentStateName.ToString();
}