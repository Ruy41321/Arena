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
}

// Called to bind functionality to input
void APlayerCharacter::SetupPlayerInputComponent(UInputComponent *PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent *EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(MoveForwardAction, ETriggerEvent::Triggered, this, &APlayerCharacter::MoveForward);
		EnhancedInputComponent->BindActionValueLambda(MoveForwardAction, ETriggerEvent::Completed, 
			[this](const FInputActionValue& Value) 
			{
				DodgeSystem->SetCurrentMovementInputAxis("X", 0.0f);
			});
		EnhancedInputComponent->BindAction(MoveRightAction, ETriggerEvent::Triggered, this, &APlayerCharacter::MoveRight);
		EnhancedInputComponent->BindActionValueLambda(MoveRightAction, ETriggerEvent::Completed, 
			[this](const FInputActionValue& Value) 
			{
				DodgeSystem->SetCurrentMovementInputAxis("Y", 0.0f);
			});
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Look);
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


void APlayerCharacter::MoveForward(const FInputActionValue &Value)
{
	const float Direction = Value.Get<float>();
	const FRotator Rotation = Controller ? Controller->GetControlRotation() : FRotator::ZeroRotator;
	const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);
	const FVector Forward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

	if (Direction != 0.0f)
	{
		// Update current movement input for dodge system
		DodgeSystem->SetCurrentMovementInputAxis("X", Direction);
		DodgeSystem->SetHasMovementInput(true);

		if (!DodgeSystem->IsDodging())
			AddMovementInput(Forward, Direction);
	}
}

void APlayerCharacter::MoveRight(const FInputActionValue &Value)
{
	const float Direction = Value.Get<float>();
	const FRotator Rotation = Controller ? Controller->GetControlRotation() : FRotator::ZeroRotator;
	const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);
	const FVector Right = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	if (Direction != 0.0f)
	{
		// Update current movement input for dodge system  
		DodgeSystem->SetCurrentMovementInputAxis("Y", Direction);
		DodgeSystem->SetHasMovementInput(true);

		if (!DodgeSystem->IsDodging())
			AddMovementInput(Right, Direction);
	}
}

void APlayerCharacter::Look(const FInputActionValue &Value)
{
	const FVector2D LookAxis = Value.Get<FVector2D>();
	if (Controller)
	{
		AddControllerYawInput(LookAxis.X);
		AddControllerPitchInput(LookAxis.Y);
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
		GetCharacterMovement()->MaxWalkSpeed = RunSpeed; // Increase speed when sprinting
	}
	else
	{
		SprintInterrupted = true;
		if (CrouchSystem && !CrouchSystem->IsCrouched())
			GetCharacterMovement()->MaxWalkSpeed = WalkSpeed; // Reset speed when not sprinting
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
	float MaxSpeed;
	
	// Check dodge system first (highest priority)
	if (DodgeSystem && DodgeSystem->IsDodging())
	{
		MaxSpeed = DodgeSystem->GetDodgeSpeed();
	}
	// Then check crouch state
	else if (CrouchSystem && CrouchSystem->IsCrouched())
	{
		MaxSpeed = CrouchSystem->GetCrouchSpeed();
	}
	// Then check sprint state
	else if (!SprintInterrupted)
	{
		MaxSpeed = RunSpeed;
	}
	// Default to walk speed
	else
	{
		MaxSpeed = WalkSpeed;
	}
	
	GetCharacterMovement()->MaxWalkSpeed = MaxSpeed;
}