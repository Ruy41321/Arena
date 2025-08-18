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
	{
		return;
	}
	Velocity = PlayerCharacter->GetCharacterMovement()->Velocity;
	Speed = Velocity.Size2D();
	IsJumping = PlayerCharacter->GetCharacterMovement()->IsFalling() and Velocity.Z > 0.0f;
	IsFalling = PlayerCharacter->GetCharacterMovement()->IsFalling() and Velocity.Z <= 0.0f;
	IsLanding = PlayerCharacter->IsLanding;
	float CrouchingTransitionTarget = PlayerCharacter->IsCrouched ? 100.0f : 0.0f;
	if (CrouchingTransitionTarget != CrouchingTransitionTime)
		FUtils::HandleGenericInterpolation(CrouchingTransitionTime, CrouchingTransitionTarget, CrouchingRate, DeltaSeconds, 0.1f);
}
