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
	
	CrouchTargetHeight = 65.0f;
	StandTargetHeight = 90.0f;
	HeightAdjustmentRate = FMath::Abs(StandTargetHeight - CrouchTargetHeight) * 10.0f; // * 10 = it needs 1/10 of a second to adjust the height

	StandMeshHeightOffset = -90.0f;
	CrouchMeshHeightOffset = -60.0f;
	MeshOffsetAdjustmentRate = FMath::Abs(StandMeshHeightOffset - CrouchMeshHeightOffset) * 10.0f; // * 10 = it needs 1/10 of a second to adjust the mesh height

	UncrouchSafetyMargin = 5.0f;
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
		const FVector2D Velocity2D = FVector2D(GetCharacterMovement()->Velocity.X, GetCharacterMovement()->Velocity.Y);

		if (Velocity2D.Size() <= 0.0f)
		{
			// No movement detected - reset all movement input
			DodgeSystem->SetCurrentMovementInput(FVector::ZeroVector);
			DodgeSystem->SetHasMovementInput(false);
		}

		if (DodgeSystem->IsDodging() && !DodgeSystem->GetDodgeDirection().IsZero())
		{
			DodgeSystem->UpdateDodgeDirection();
			AddMovementInput(DodgeSystem->GetDodgeDirection(), 1.0f);
		}
		
		// Adjust capsule height based on crouching state
		float TargetCapsuleHeight = IsCrouched ? CrouchTargetHeight : StandTargetHeight;
		float TargetMeshHeight = IsCrouched ? CrouchMeshHeightOffset : StandMeshHeightOffset;
		if (GetCapsuleComponent() && IsCrouchingInProgress)
			AdjustCapsuleHeight(DeltaTime, TargetCapsuleHeight, TargetMeshHeight);
	}
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
		EnhancedInputComponent->BindAction(CrouchPressedAction, ETriggerEvent::Started, this, &APlayerCharacter::CrouchPressed);
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
		if (IsCrouched)
		{
			if (!CanUncrouchSafely())
				return;
			CrouchPressed(Value);
		}
		GetCharacterMovement()->MaxWalkSpeed = RunSpeed; // Increase speed when sprinting
	}
	else
	{
		SprintInterrupted = true;
		if (!IsCrouched)
			GetCharacterMovement()->MaxWalkSpeed = WalkSpeed; // Reset speed when not sprinting
	}
}

void APlayerCharacter::JumpPressed(const FInputActionValue &Value)
{
	if (!IsLanding && !DodgeSystem->IsDodging())
	{
		if (!IsCrouched)
			Jump();
		else
			CrouchPressed(Value);
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

void APlayerCharacter::CrouchPressed(const FInputActionValue &Value)
{
	if (DodgeSystem->IsDodging())
		return;
	if (IsCrouched)
	{
		if (!CanUncrouchSafely())
			return;
		UnCrouch();
	}
	else
		Crouch();
	IsCrouchingInProgress = true;
}

void APlayerCharacter::Crouch(bool bClientSimulation)
{
	if (GetCharacterMovement()->IsFalling())
		return;
	IsCrouched = true;
	GetCharacterMovement()->MaxWalkSpeed = CrouchSpeed;
}

void APlayerCharacter::UnCrouch(bool bClientSimulation)
{
	IsCrouched = false;
	GetCharacterMovement()->MaxWalkSpeed = !SprintInterrupted ? RunSpeed : WalkSpeed;
}

void APlayerCharacter::AdjustCapsuleHeight(float DeltaTime, float TargetCapsuleHeight, float TargetMeshHeight)
{
	bool IsFinished = true;
	const float Tolerance = 0.01f;

	// Adjusting the capsule height
	if (GetCapsuleComponent())
	{
		float CurrentHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

		if (!FUtils::HandleGenericInterpolation(CurrentHeight, TargetCapsuleHeight, HeightAdjustmentRate, DeltaTime, Tolerance))
			IsFinished = false;
		GetCapsuleComponent()->SetCapsuleHalfHeight(CurrentHeight);
	}

	// Adjusting the mesh position
	if (GetMesh())
	{
		FVector CurrentLocation = GetMesh()->GetRelativeLocation();
		float CurrentMeshHeight = CurrentLocation.Z;

		if (!FUtils::HandleGenericInterpolation(CurrentMeshHeight, TargetMeshHeight, MeshOffsetAdjustmentRate, DeltaTime, Tolerance))
			IsFinished = false;
		CurrentLocation.Z = CurrentMeshHeight; // Update the Z position of the mesh
		GetMesh()->SetRelativeLocation(CurrentLocation);
	}

	//UE_LOG(LogTemp, Log, TEXT("AdjustCapsuleHeight: Capsule Height: %f, Mesh Height: %f"),
	//	   GetCapsuleComponent()->GetScaledCapsuleHalfHeight(), GetMesh()->GetRelativeLocation().Z);

	IsCrouchingInProgress = !IsFinished;
}

bool APlayerCharacter::CanUncrouchSafely() const
{
	if (!GetCapsuleComponent() || !GetWorld() || !IsCrouched)
	{
		return false;
	}

	// Get current capsule properties
	const float CurrentCapsuleRadius = GetCapsuleComponent()->GetScaledCapsuleRadius();
	const float CurrentCapsuleHalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	
	// Calculate the target standing height
	const float TargetCapsuleHalfHeight = StandTargetHeight;
	const float HeightDifference = TargetCapsuleHalfHeight - CurrentCapsuleHalfHeight;
	
	// If we're already at standing height or taller, no need to check
	if (HeightDifference <= 0.0f)
	{
		return true;
	}

	// Get current location
	const FVector CurrentLocation = GetActorLocation();
	
	// Create a capsule shape for the standing position with safety margin
	const FCollisionShape CapsuleShape = FCollisionShape::MakeCapsule(
		CurrentCapsuleRadius, 
		TargetCapsuleHalfHeight + UncrouchSafetyMargin
	);
	
	// Calculate the position where the capsule would be when standing
	// The capsule center needs to be adjusted upward by the height difference plus safety margin
	const FVector TargetLocation = CurrentLocation + FVector(0.0f, 0.0f, HeightDifference + UncrouchSafetyMargin);
	
	// Set up collision query parameters
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this); // Ignore self
	QueryParams.bTraceComplex = false; // Use simple collision for performance
	
	// Perform overlap test at the target standing position
	const bool bHasOverlap = GetWorld()->OverlapBlockingTestByChannel(
		TargetLocation,
		FQuat::Identity,
		ECollisionChannel::ECC_Pawn, // Use pawn collision channel
		CapsuleShape,
		QueryParams
	);
	
	// Log for debugging purposes
	UE_LOG(LogTemp, VeryVerbose, TEXT("CanUncrouchSafely: HeightDiff=%.2f, HasOverlap=%s"), 
		   HeightDifference, bHasOverlap ? TEXT("true") : TEXT("false"));
	
	// If there's no overlap, we can safely uncrouch
	return !bHasOverlap;
}

void APlayerCharacter::UpdateMaxWalkSpeed()
{
	float MaxSpeed;
	if (DodgeSystem->IsDodging())
		MaxSpeed = DodgeSystem->GetDodgeSpeed();
	else if (IsCrouched)
		MaxSpeed = CrouchSpeed;
	else if (!SprintInterrupted)
		MaxSpeed = RunSpeed;
	else
		MaxSpeed = WalkSpeed;
	GetCharacterMovement()->MaxWalkSpeed = MaxSpeed;
}