// Copyright (c) 2025 Luigi Pennisi. All rights reserved.

#include "PlayerCharacter.h"
#include "../PlayerAnimation/PlayerAnimInstance.h"

APlayerCharacter::APlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	UCharacterMovementComponent *const mov = GetCharacterMovement();
	if (mov)
		mov->bOrientRotationToMovement = true;
	bUseControllerRotationYaw = false;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("Camera Boom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->bUsePawnControlRotation = true;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Player Camera"));
	Camera->SetupAttachment(CameraBoom);

	DodgeSystem = CreateDefaultSubobject<UDodgeSystemComponent>(TEXT("Dodge System Component"));
	CrouchSystem = CreateDefaultSubobject<UCrouchSystemComponent>(TEXT("Crouch System Component"));
	BasicMovementSystem = CreateDefaultSubobject<UBasicMovementComponent>(TEXT("Basic Movement Component"));
	JumpSystem = CreateDefaultSubobject<UJumpSystemComponent>(TEXT("Jump System Component"));
	SprintSystem = CreateDefaultSubobject<USprintSystemComponent>(TEXT("Sprint System Component"));
	MovementStateMachine = CreateDefaultSubobject<UMovementStateMachine>(TEXT("Movement State Machine"));
}

void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	if (APlayerController *PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem *Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void APlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void APlayerCharacter::SetupPlayerInputComponent(UInputComponent *PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent *EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Delegate input setup to each component
		if (BasicMovementSystem)
			BasicMovementSystem->SetupInput(EnhancedInputComponent);

		if (SprintSystem)
			SprintSystem->SetupInput(EnhancedInputComponent);

		if (JumpSystem)
			JumpSystem->SetupInput(EnhancedInputComponent);

		if (CrouchSystem)
			CrouchSystem->SetupInput(EnhancedInputComponent);

		if (DodgeSystem)
			DodgeSystem->SetupInput(EnhancedInputComponent);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayerInputComponent is not of type UEnhancedInputComponent"));
	}
}

void APlayerCharacter::Landed(const FHitResult &Hit)
{
	Super::Landed(Hit);
	
	// Delegate landing logic to JumpSystemComponent
	if (JumpSystem)
	{
		JumpSystem->OnLanded(Hit);
	}
}

void APlayerCharacter::SetMaxWalkSpeed(float NewSpeed)
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (Movement)
	{
		Movement->MaxWalkSpeed = NewSpeed;
		Movement->MaxWalkSpeedCrouched = NewSpeed;
		UE_LOG(LogTemp, Verbose, TEXT("PlayerCharacter: MaxWalkSpeed set to %f"), NewSpeed);
	}
}

EMovementState APlayerCharacter::GetCurrentMovementState() const
{
	if (MovementStateMachine)
	{
		return MovementStateMachine->GetCurrentState();
	}
	return EMovementState::None;
}

EMovementState APlayerCharacter::GetPreviousMovementState() const
{
	if (MovementStateMachine)
	{
		return MovementStateMachine->GetPreviousState();
	}
	return EMovementState::None;
}

bool APlayerCharacter::TransitionToMovementState(EMovementState NewState, bool bForceTransition)
{
	if (MovementStateMachine)
	{
		return MovementStateMachine->TransitionToState(NewState, bForceTransition);
	}
	return false;
}

FString APlayerCharacter::GetCurrentMovementStateAsString() const
{
	return UMovementStateTypes::MovementStateToString(GetCurrentMovementState());
}

void APlayerCharacter::UnsubscribeFromMovementStateChanges(UObject* Subscriber)
{
	if (MovementStateMachine)
	{
		MovementStateMachine->UnsubscribeFromStateChanges(Subscriber);
	}
}