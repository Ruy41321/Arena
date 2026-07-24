// Copyright (c) 2025 Luigi Pennisi. All rights reserved.

#include "Player/MKHPlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/Controller.h"

#include "Player/PlayerAnimation/MKHPlayerAnimInstance.h"
#include "Player/Components/Dodge/DodgeSystemComponent.h"
#include "Player/Components/Crouch/CrouchSystemComponent.h"
#include "Player/Components/Blocking/BlockingSystemComponent.h"
#include "Player/Components/BasicMovement/BasicMovementComponent.h"
#include "Player/Components/Jump/JumpSystemComponent.h"
#include "Player/Components/Sprint/SprintSystemComponent.h"
#include "Player/MovementStateMachine/MovementStateMachine.h"

#include "Player/PlayerState/MKHPlayerState.h"
#include "GameInstance/MKHGameInstance.h"
#include "AbilitySystem/MKHAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/MKHAttributeSet.h"
#include "AbilitySystem/MKHGameplayTags.h"
#include "Net/UnrealNetwork.h"

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
	BlockingSystem = CreateDefaultSubobject<UBlockingSystemComponent>(TEXT("Blocking System Component"));
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
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayerInputComponent is not of type UEnhancedInputComponent"));
	}
}

void AMKHPlayerCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(AMKHPlayerCharacter, bIsHoldingWeapon);
	DOREPLIFETIME(AMKHPlayerCharacter, Username);
}

void AMKHPlayerCharacter::SetUsername(const FString& NewUsername)
{
	// Only the authority may commit the replicated value; a client routes it through the server.
	if (!HasAuthority())
	{
		Server_SetUsername(NewUsername);
		return;
	}

	if (Username == NewUsername)
	{
		return;
	}

	Username = NewUsername;

	// OnRep never runs on the authority, so the server notifies itself here. Clients are notified
	// by OnRep_Username once the new value replicates in.
	OnUsernameChanged.Broadcast(Username);
}

void AMKHPlayerCharacter::Server_SetUsername_Implementation(const FString& NewUsername)
{
	// Re-enter through the setter so the commit and the notification stay in one place.
	SetUsername(NewUsername);
}

void AMKHPlayerCharacter::OnRep_Username()
{
	OnUsernameChanged.Broadcast(Username);
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

void AMKHPlayerCharacter::Multicast_TransitionToMovementState_Implementation(EMovementStateValue NewState,
	bool bForceTransition)
{
	TransitionToMovementState(NewState, bForceTransition);
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

	// Authority path (listen-server host): the controller is now set, so the local game instance
	// username can be committed directly.
	TrySeedUsernameFromLocalGameInstance();
}

void AMKHPlayerCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	InitAbilityActorInfo();
	BroadcastInitialValues();
}

void AMKHPlayerCharacter::OnRep_Controller()
{
	Super::OnRep_Controller();

	// Re-init only when a previous init cached a null PlayerController (PlayerState
	// replicated first). The guard keeps the common path from re-running the init and
	// double-binding attribute delegates. When the ASC is still null the controller arrived
	// first: OnRep_PlayerState will run the init with the controller already in place.
	if (IsValid(MKHAbilitySystemComponent)
		&& MKHAbilitySystemComponent->AbilityActorInfo.IsValid()
		&& !MKHAbilitySystemComponent->AbilityActorInfo->PlayerController.IsValid())
	{
		InitAbilityActorInfo();
	}

	// Owning client: the controller has now replicated, so the pawn is locally controlled and owned
	// by the local player controller — the ownership required to route the username server RPC.
	TrySeedUsernameFromLocalGameInstance();
}

void AMKHPlayerCharacter::TrySeedUsernameFromLocalGameInstance()
{
	// Run once, and only for the pawn the local player actually controls: a simulated proxy would
	// otherwise seed the wrong machine's username, and its server RPC would be dropped for lack of ownership.
	if (bUsernameSeeded || !IsLocallyControlled())
	{
		return;
	}

	if (const UMKHGameInstance* GameInstance = UMKHGameInstance::GetMKHGameInstance(this))
	{
		bUsernameSeeded = true;
		SetUsername(GameInstance->GetUsername());
	}
}

void AMKHPlayerCharacter::Multicast_StopMontage_Implementation(UAnimMontage* MontageToStop, const float InBlendOutTime)
{
	if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
	{
		// Stop only the requested montage: a late-arriving stop for a cancelled ability
		// must never kill the montage of a different ability that has since started.
		AnimInstance->Montage_Stop(InBlendOutTime, MontageToStop);
	}
}

void AMKHPlayerCharacter::Multicast_ResetToSpawnTransform_Implementation(const FTransform& SpawnTransform)
{
	// Clear leftover velocity and pending input first: with bOrientRotationToMovement the movement
	// component keeps steering the actor towards its acceleration, which would undo the rotation
	// we are about to apply within the very next frame.
	if (UCharacterMovementComponent* MovementComp = GetCharacterMovement())
	{
		MovementComp->StopMovementImmediately();
	}

	SetActorTransform(SpawnTransform, false, nullptr, ETeleportType::TeleportPhysics);
	ApplySpawnViewRotation(SpawnTransform.Rotator());
}

void AMKHPlayerCharacter::ApplySpawnViewRotation(const FRotator& SpawnRotation)
{
	AController* OwningController = GetController();
	if (!IsValid(OwningController) || (!IsLocallyControlled() && !HasAuthority()))
	{
		return;
	}

	// Yaw only: pitch and roll belong to the camera, not to the spawn orientation.
	OwningController->SetControlRotation(FRotator(0.f, SpawnRotation.Yaw, 0.f));
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
		// Rebind-safe, same rationale as ACharacterBase::BindCallbacksToDependencies.
		MKHAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UMKHAttributeSet::GetStaminaAttribute()).RemoveAll(this);

		// Stamina
		MKHAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UMKHAttributeSet::GetStaminaAttribute()).AddWeakLambda(this,
			[this](const FOnAttributeChangeData& Data)
			{
				HandleStaminaChanged(Data.OldValue, Data.NewValue, MKHAttributeSet->GetMaxStamina());
				OnStaminaChanged.Broadcast(Data.OldValue, Data.NewValue, MKHAttributeSet->GetMaxStamina());
			});

		// Crowd control (Stunned / Staggered): cancel ongoing movement actions when applied.
		// Rebind-safe: BindCallbacksToDependencies can run again on controller/state re-init.
		MKHAbilitySystemComponent->RegisterGameplayTagEvent(
			MKHGameplayTags::State::Stunned, EGameplayTagEventType::NewOrRemoved).RemoveAll(this);
		MKHAbilitySystemComponent->RegisterGameplayTagEvent(
			MKHGameplayTags::State::Stunned, EGameplayTagEventType::NewOrRemoved).AddUObject(
				this, &AMKHPlayerCharacter::HandleCrowdControlTagChanged);

		MKHAbilitySystemComponent->RegisterGameplayTagEvent(
			MKHGameplayTags::State::Staggered, EGameplayTagEventType::NewOrRemoved).RemoveAll(this);
		MKHAbilitySystemComponent->RegisterGameplayTagEvent(
			MKHGameplayTags::State::Staggered, EGameplayTagEventType::NewOrRemoved).AddUObject(
				this, &AMKHPlayerCharacter::HandleCrowdControlTagChanged);
	}
}

void AMKHPlayerCharacter::HandleCrowdControlTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	// Only react when the crowd-control effect becomes active; ignore its removal.
	if (NewCount <= 0)
		return;

	// Force the character out of crouch (respecting headroom).
	if (IsValid(CrouchSystem))
	{
		CrouchSystem->ForceUncrouch();
	}

	// Interrupt any in-progress jump so upward movement stops immediately.
	if (IsValid(JumpSystem))
	{
		JumpSystem->InterruptJump();
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
