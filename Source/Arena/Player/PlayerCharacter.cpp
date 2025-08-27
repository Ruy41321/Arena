// Fill out your copyright notice in the Description page of Project Settings.

#include "PlayerCharacter.h"
#include "../PlayerAnimation/PlayerAnimInstance.h"

APlayerCharacter::APlayerCharacter()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	UCharacterMovementComponent *const mov = GetCharacterMovement();
	if (mov)
		mov->bOrientRotationToMovement = true; // Character will rotate to movement direction
	bUseControllerRotationYaw = false;		   // Disable controller rotation yaw to allow character movement direction to control rotation

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("Camera Boom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->bUsePawnControlRotation = true;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Player Camera"));
	Camera->SetupAttachment(CameraBoom);

	DodgeSystem = CreateDefaultSubobject<UDodgeSystemComponent>(TEXT("Dodge System Component"));
	CrouchSystem = CreateDefaultSubobject<UCrouchSystemComponent>(TEXT("Crouch System Component"));
	MovementInputSystem = CreateDefaultSubobject<UMovementInputComponent>(TEXT("Movement Input Component"));
}

// Called when the game starts or when spawned
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

// Called every frame
void APlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	if (GetCharacterMovement())
	{
		// Handle dodge movement
		if (DodgeSystem && DodgeSystem->IsDodging() && !DodgeSystem->GetDodgeDirection().IsZero())
		{
			DodgeSystem->UpdateDodgeDirection();
			AddMovementInput(DodgeSystem->GetDodgeDirection(), 1.0f);
		}
		
		// Handle crouch height adjustment
		if (CrouchSystem && CrouchSystem->IsCrouchingInProgress())
		{
			float TargetCapsuleHeight = CrouchSystem->IsCrouched() ? CrouchSystem->GetCrouchTargetHeight() : CrouchSystem->GetStandTargetHeight();
			float TargetMeshHeight = CrouchSystem->IsCrouched() ? CrouchSystem->GetCrouchMeshHeightOffset() : CrouchSystem->GetStandMeshHeightOffset();
			CrouchSystem->AdjustCapsuleHeight(DeltaTime, TargetCapsuleHeight, TargetMeshHeight);
		}
	}
}

// Called to bind functionality to input
void APlayerCharacter::SetupPlayerInputComponent(UInputComponent *PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent *EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Bind movement input to MovementInputSystem using lambdas
		EnhancedInputComponent->BindActionValueLambda(MoveForwardAction, ETriggerEvent::Triggered, 
			[this](const FInputActionValue& Value) {
				if (MovementInputSystem)
					MovementInputSystem->MoveForward(Value);
			});
		EnhancedInputComponent->BindActionValueLambda(MoveForwardAction, ETriggerEvent::Completed, 
			[this](const FInputActionValue& Value) 
			{
				if (MovementInputSystem)
					MovementInputSystem->OnMovementInputCompleted("X");
			});
		EnhancedInputComponent->BindActionValueLambda(MoveRightAction, ETriggerEvent::Triggered, 
			[this](const FInputActionValue& Value) {
				if (MovementInputSystem)
					MovementInputSystem->MoveRight(Value);
			});
		EnhancedInputComponent->BindActionValueLambda(MoveRightAction, ETriggerEvent::Completed, 
			[this](const FInputActionValue& Value) 
			{
				if (MovementInputSystem)
					MovementInputSystem->OnMovementInputCompleted("Y");
			});
		EnhancedInputComponent->BindActionValueLambda(LookAction, ETriggerEvent::Triggered, 
			[this](const FInputActionValue& Value) {
				if (MovementInputSystem)
					MovementInputSystem->Look(Value);
			});
		
		// Keep other actions bound to PlayerCharacter
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &APlayerCharacter::Sprint);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &APlayerCharacter::Sprint);
		EnhancedInputComponent->BindAction(JumpPressedAction, ETriggerEvent::Started, this, &APlayerCharacter::JumpPressed);
		EnhancedInputComponent->BindAction(JumpPressedAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
		EnhancedInputComponent->BindActionValueLambda(CrouchPressedAction, ETriggerEvent::Started, 
			[this](const FInputActionValue& Value) {
				if (CrouchSystem)
					CrouchSystem->CrouchPressed(Value);
			});
		EnhancedInputComponent->BindActionValueLambda(DodgeAction, ETriggerEvent::Started,
			[this](const FInputActionValue& Value) {
				if (DodgeSystem)
					DodgeSystem->StartDodge();
			});
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayerInputComponent is not of type UEnhancedInputComponent"));
	}
}

void APlayerCharacter::Sprint(const FInputActionValue &Value)
{
	const bool SprintValue = Value.Get<bool>();
	if (SprintValue)
	{
		SprintInterrupted = false;
		if (DodgeSystem->IsDodging())
			return;
		if (CrouchSystem && CrouchSystem->IsCrouched())
		{
			if (!CrouchSystem->CanUncrouchSafely())
				return;
			CrouchSystem->CrouchPressed(Value);
		}
		// Use MovementInputSystem for speed updates
		if (MovementInputSystem)
			MovementInputSystem->UpdateMaxWalkSpeed();
	}
	else
	{
		SprintInterrupted = true;
		// Use MovementInputSystem for speed updates
		if (MovementInputSystem)
			MovementInputSystem->UpdateMaxWalkSpeed();
	}
}

void APlayerCharacter::JumpPressed(const FInputActionValue &Value)
{
	if (!IsLanding && !DodgeSystem->IsDodging())
	{
		if (!CrouchSystem || !CrouchSystem->IsCrouched())
			Jump();
		else
			CrouchSystem->CrouchPressed(Value);
	}
}

void APlayerCharacter::Landed(const FHitResult &Hit)
{
	Super::Landed(Hit);
	IsLanding = true;
	// set a timer of x seconds to reset IsLanding
	GetWorld()->GetTimerManager().SetTimer(LandingTimerHandle, this, &APlayerCharacter::ResetLanding, 0.1f, false);
}

void APlayerCharacter::ResetLanding()
{
	IsLanding = false;
}

void APlayerCharacter::UpdateMaxWalkSpeed()
{
	// Delegate to MovementInputSystem which now handles all movement speed logic
	if (MovementInputSystem)
	{
		MovementInputSystem->UpdateMaxWalkSpeed();
	}
}