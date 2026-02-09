// Copyright (c) 2025 Luigi Pennisi. All rights reserved.

#include "Player/PlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputActionValue.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Components/InputComponent.h"
#include "Components/CapsuleComponent.h"

#include "Utils/Utils.h"
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
#include "Libraries/RPGAbilitySystemLibrary.h"
#include "Data/CharacterClassInfo.h"

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

UAbilitySystemComponent* APlayerCharacter::GetAbilitySystemComponent() const
{
	return RPGAbilitySystemComponent;
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

			if (HasAuthority())
			{
				InitClassDefaults();
			}
		}
	}
}

void APlayerCharacter::InitClassDefaults()
{
	if (!CharacterTag.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("No CharacterTag Selected In This Character %s"), *GetNameSafe(this));
	}
	else if (UCharacterClassInfo* ClassInfo = URPGAbilitySystemLibrary::GetCharacterClassDefaultInfo(this))
	{
		if (FCharacterClassDefaultInfo* SelectorClassInfo = ClassInfo->ClassDefaultInfoMap.Find(CharacterTag))
		{
			if (IsValid(RPGAbilitySystemComponent))
			{
				RPGAbilitySystemComponent->AddCharacterAbilities(SelectorClassInfo->StartingAbilities);
				RPGAbilitySystemComponent->AddCharacterPassiveAbilities(SelectorClassInfo->StartingPassives);
				RPGAbilitySystemComponent->initializeDefaultAttributes(SelectorClassInfo->DefaultAttributes);
			}
		}
	}
}

void APlayerCharacter::BindCallbacksToDependencies()
{
	if (IsValid(RPGAbilitySystemComponent) && IsValid(RPGAttributeSet))
	{
		//Health
		RPGAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(URPGAttributeSet::GetHealthAttribute()).AddLambda(
			[this](const FOnAttributeChangeData& Data)
			{
				OnHealthChanged(Data.NewValue, RPGAttributeSet->GetMaxHealth());
			});
		//Mana
		RPGAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(URPGAttributeSet::GetManaAttribute()).AddLambda(
			[this](const FOnAttributeChangeData& Data)
			{
				OnManaChanged(Data.NewValue, RPGAttributeSet->GetMaxMana());
			});
	}
}

void APlayerCharacter::BroadcastInitialValues()
{
	if (IsValid(RPGAttributeSet))
	{
		OnHealthChanged(RPGAttributeSet->GetHealth(), RPGAttributeSet->GetMaxHealth());
		OnManaChanged(RPGAttributeSet->GetMana(), RPGAttributeSet->GetMaxMana());
	}
}
