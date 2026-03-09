// Copyright (c) 2025 Luigi Pennisi. All rights reserved.

#include "Player/PlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Components/InputComponent.h"

#include "Player/PlayerAnimation/PlayerAnimInstance.h"
#include "Player/Components/Dodge/DodgeSystemComponent.h"
#include "Player/Components/Crouch/CrouchSystemComponent.h"
#include "Player/Components/BasicMovement/BasicMovementComponent.h"
#include "Player/Components/Jump/JumpSystemComponent.h"
#include "Player/Components/Sprint/SprintSystemComponent.h"
#include "Player/MovementStateMachine/MovementStateMachine.h"

#include "Player/PlayerState/RPGPlayerState.h"
#include "AbilitySystem/RPGAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/RPGAttributeSet.h"

APlayerCharacter::APlayerCharacter()
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

	DynamicProjectileSpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("Projectile Spawn Point"));
	DynamicProjectileSpawnPoint->SetupAttachment(GetRootComponent());
}

void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	// Ensure mesh replicates smoothly on clients
	if (GetMesh())
	{
		GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
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

		// Commented because handling it with Gameplay Ability System
		// if (DodgeSystem)
		// 	DodgeSystem->SetupInput(EnhancedInputComponent);
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

EMovementStateValue APlayerCharacter::GetCurrentMovementState() const
{
	if (MovementStateMachine)
	{
		return MovementStateMachine->GetCurrentState();
	}
	return EMovementStateValue::None;
}

EMovementStateValue APlayerCharacter::GetPreviousMovementState() const
{
	if (MovementStateMachine)
	{
		return MovementStateMachine->GetPreviousState();
	}
	return EMovementStateValue::None;
}

bool APlayerCharacter::TransitionToMovementState(EMovementStateValue NewState, bool bForceTransition)
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

void APlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (HasAuthority())
	{
		InitAbilityActorInfo();
	}
}

void APlayerCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	InitAbilityActorInfo();
}


void APlayerCharacter::InitAbilityActorInfo()
{
	if (ARPGPlayerState* RPGPlayerState = GetPlayerState<ARPGPlayerState>())
	{
		RPGAbilitySystemComponent = RPGPlayerState->GetRPGAbilitySystemComponent();
		RPGAttributeSet = RPGPlayerState->GetRPGAttributeSet();

		if (IsValid(RPGAbilitySystemComponent))
		{
			RPGAbilitySystemComponent->InitAbilityActorInfo(RPGPlayerState, this);
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

void APlayerCharacter::InitClassDefaults()
{
	// La logica comune (AddCharacterAbilities, InitializeDefaultAttributes, ecc.) è in CharacterBase::InitClassDefaults
	Super::InitClassDefaults();
}

void APlayerCharacter::BindCallbacksToDependencies()
{
	// Delegate comuni (Health, Shield) gestite da CharacterBase
	Super::BindCallbacksToDependencies();

	// Stamina: specifica del player
	if (IsValid(RPGAbilitySystemComponent) && IsValid(RPGAttributeSet))
	{
		//Stamina
		RPGAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(URPGAttributeSet::GetStaminaAttribute()).AddLambda(
			[this](const FOnAttributeChangeData& Data)
			{
				OnStaminaChanged(Data.OldValue, Data.NewValue, RPGAttributeSet->GetMaxStamina());
			});
	}
}

void APlayerCharacter::BroadcastInitialValues()
{
	Super::BroadcastInitialValues();
	
	if (IsValid(RPGAttributeSet))
	{
		OnStaminaChanged(RPGAttributeSet->GetStamina(), RPGAttributeSet->GetStamina(), RPGAttributeSet->GetMaxStamina());
	}
}

USceneComponent* APlayerCharacter::GetDynamicSpawnPoint_Implementation()
{
	return DynamicProjectileSpawnPoint;
}