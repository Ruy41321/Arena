// Copyright (c) 2025 Luigi Pennisi. All rights reserved.

#include "Player/MKHPlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "Components/InputComponent.h"

#include "Player/PlayerAnimation/MKHPlayerAnimInstance.h"
#include "Player/Components/Dodge/DodgeSystemComponent.h"
#include "Player/Components/Crouch/CrouchSystemComponent.h"
#include "Player/Components/BasicMovement/BasicMovementComponent.h"
#include "Player/Components/Jump/JumpSystemComponent.h"
#include "Player/Components/Sprint/SprintSystemComponent.h"
#include "Player/MovementStateMachine/MovementStateMachine.h"

#include "Player/PlayerState/MKHPlayerState.h"
#include "AbilitySystem/MKHAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/MKHAttributeSet.h"

AMKHPlayerCharacter::AMKHPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	UCharacterMovementComponent *const mov = GetCharacterMovement();
	if (mov)
	{
		mov->bOrientRotationToMovement = true;
	}
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

void AMKHPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	// Ensure mesh replicates smoothly on clients
	if (GetMesh())
	{
		GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	}
}

void AMKHPlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AMKHPlayerCharacter::SetupPlayerInputComponent(UInputComponent *PlayerInputComponent)
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

		// Commented because handling it with Gameplay Ability System
		// if (DodgeSystem)
		// 	DodgeSystem->SetupInput(EnhancedInputComponent);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayerInputComponent is not of type UEnhancedInputComponent"));
	}
}

void AMKHPlayerCharacter::Landed(const FHitResult &Hit)
{
	Super::Landed(Hit);
	
	// Delegate landing logic to JumpSystemComponent
	if (JumpSystem)
	{
		JumpSystem->OnLanded(Hit);
	}
}

void AMKHPlayerCharacter::SetMaxWalkSpeed(float NewSpeed)
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (Movement)
	{
		Movement->MaxWalkSpeed = NewSpeed;
		Movement->MaxWalkSpeedCrouched = NewSpeed;
		UE_LOG(LogTemp, Verbose, TEXT("MKHPlayerCharacter: MaxWalkSpeed set to %f"), NewSpeed);
	}
}

EMovementStateValue AMKHPlayerCharacter::GetCurrentMovementState() const
{
	if (MovementStateMachine)
	{
		return MovementStateMachine->GetCurrentState();
	}
	return EMovementStateValue::None;
}

EMovementStateValue AMKHPlayerCharacter::GetPreviousMovementState() const
{
	if (MovementStateMachine)
	{
		return MovementStateMachine->GetPreviousState();
	}
	return EMovementStateValue::None;
}

bool AMKHPlayerCharacter::TransitionToMovementState(EMovementStateValue NewState, bool bForceTransition)
{
	if (MovementStateMachine)
	{
		return MovementStateMachine->TransitionToState(NewState, bForceTransition);
	}
	return false;
}

FString AMKHPlayerCharacter::GetCurrentMovementStateAsString() const
{
	return UMovementStateTypes::MovementStateToString(GetCurrentMovementState());
}

void AMKHPlayerCharacter::UnsubscribeFromMovementStateChanges(UObject* Subscriber)
{
	if (MovementStateMachine)
	{
		MovementStateMachine->UnsubscribeFromStateChanges(Subscriber);
	}
}

void AMKHPlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (HasAuthority())
	{
		InitAbilityActorInfo();
		BroadcastInitialValues();
	}
}

void AMKHPlayerCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	InitAbilityActorInfo();
	BroadcastInitialValues();
}


void AMKHPlayerCharacter::InitAbilityActorInfo()
{
	if (AMKHPlayerState* MKHPlayerState = GetPlayerState<AMKHPlayerState>())
	{
		MKHAbilitySystemComponent = MKHPlayerState->GetRPGAbilitySystemComponent();
		MKHAttributeSet = MKHPlayerState->GetRPGAttributeSet();

		if (IsValid(MKHAbilitySystemComponent))
		{
			MKHAbilitySystemComponent->InitAbilityActorInfo(MKHPlayerState, this);
			BindCallbacksToDependencies();

			// The movement state machine sets its initial state during BeginPlay,
			// before the ASC is available, so the gameplay tag was never added.
			// Sync it now that the ASC is ready.
			if (MovementStateMachine)
			{
				MovementStateMachine->SyncCurrentStateTagToASC();
			}

			if (HasAuthority())
			{
				InitClassDefaults();
			}
		}
	}
}

void AMKHPlayerCharacter::InitClassDefaults()
{
	// La logica comune (AddCharacterAbilities, InitializeDefaultAttributes, ecc.) ÃƒÆ’Ã‚Â¨ in CharacterBase::InitClassDefaults
	Super::InitClassDefaults();
}

void AMKHPlayerCharacter::BindCallbacksToDependencies()
{
	// Delegate comuni (Health, Shield) gestite da CharacterBase
	Super::BindCallbacksToDependencies();

	// Stamina: specifica del player
	if (IsValid(MKHAbilitySystemComponent) && IsValid(MKHAttributeSet))
	{
		// Stamina
		MKHAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UMKHAttributeSet::GetStaminaAttribute()).AddLambda(
			[this](const FOnAttributeChangeData& Data)
			{
				HandleStaminaChanged(Data.OldValue, Data.NewValue, MKHAttributeSet->GetMaxStamina());
				OnStaminaChanged.Broadcast(Data.OldValue, Data.NewValue, MKHAttributeSet->GetMaxStamina());
			});
	}
}

void AMKHPlayerCharacter::BroadcastInitialValues()
{
	Super::BroadcastInitialValues();
	
	if (IsValid(MKHAttributeSet))
	{
		OnStaminaChanged.Broadcast(MKHAttributeSet->GetStamina(), MKHAttributeSet->GetStamina(), MKHAttributeSet->GetMaxStamina());
	}
}
