// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerAnimInstance.h"
#include "GameFramework/CharacterMovementComponent.h"


void UPlayerAnimInstance::NativeBeginPlay()
{
	Super::NativeBeginPlay();
	PlayerCharacter = dynamic_cast<APlayerCharacter*>(TryGetPawnOwner());
	if (GetWorld() && GetWorld()->IsGameWorld())
		UE_LOG(LogTemp, Warning, TEXT("Player: %s"), *PlayerCharacter->GetName());
	Speed = 0.0f;
}

void UPlayerAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	if (GetWorld() && GetWorld()->IsGameWorld())
		UE_LOG(LogTemp, Warning, TEXT("Sono qui"));
	if (!PlayerCharacter)
	{
		return;
	}
	Velocity = PlayerCharacter->GetCharacterMovement()->Velocity;
	Speed = Velocity.Size2D();
	IsSprinting = PlayerCharacter->IsSprinting;
	IsJumping = PlayerCharacter->GetCharacterMovement()->IsFalling() and Velocity.Z > 0.0f;
	IsFalling = PlayerCharacter->GetCharacterMovement()->IsFalling() and Velocity.Z <= 0.0f;
	IsLanding = PlayerCharacter->IsLanding;
}