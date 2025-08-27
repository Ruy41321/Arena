// Fill out your copyright notice in the Description page of Project Settings.

#include "MovementInputComponent.h"
#include "../../Player/PlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

// Sets default values for this component's properties
UMovementInputComponent::UMovementInputComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// Initialize default values
	MouseSensitivity = 1.0f;
	bInvertYAxis = false;
	MovementSmoothing = 0.1f;
}


// Called when the game starts
void UMovementInputComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerPlayerCharacter = Cast<APlayerCharacter>(GetOwner());

	// Validation for safety in development
	if (!OwnerPlayerCharacter)
	{
		UE_LOG(LogTemp, Error, TEXT("MovementInputComponent: Owner is not a PlayerCharacter! Owner class: %s"),
			GetOwner() ? *GetOwner()->GetClass()->GetName() : TEXT("NULL"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("MovementInputComponent: Successfully cached PlayerCharacter - %s at address %p"),
			*OwnerPlayerCharacter->GetName(), static_cast<void*>(OwnerPlayerCharacter.Get()));
	}
}


// Called every frame
void UMovementInputComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Update movement velocity tracking
	UpdateMovementVelocity();
}


FORCEINLINE APlayerCharacter* UMovementInputComponent::GetValidPlayerCharacter() const
{
	// Double validation for maximum safety with optimal branch prediction
	if (LIKELY(OwnerPlayerCharacter && IsValid(OwnerPlayerCharacter)))
	{
		return OwnerPlayerCharacter;
	}

	// Fallback: re-cache if necessary (should be very rare)
	return Cast<APlayerCharacter>(GetOwner());
}

void UMovementInputComponent::MoveForward(const FInputActionValue& Value)
{
	APlayerCharacter* PlayerCharacter = GetValidPlayerCharacter();
	if (!PlayerCharacter)
		return;

	const float Direction = Value.Get<float>();
	const FRotator Rotation = PlayerCharacter->GetController() ? PlayerCharacter->GetController()->GetControlRotation() : FRotator::ZeroRotator;
	const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);
	const FVector Forward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

	if (Direction != 0.0f)
	{
		// Update current movement input for other systems (like dodge system)
		SetCurrentMovementInputAxis("X", Direction);
		SetHasMovementInput(true);

		// Only add movement input if not dodging
		if (PlayerCharacter->DodgeSystem && !PlayerCharacter->DodgeSystem->IsDodging())
			PlayerCharacter->AddMovementInput(Forward, Direction);
	}
}

void UMovementInputComponent::MoveRight(const FInputActionValue& Value)
{
	APlayerCharacter* PlayerCharacter = GetValidPlayerCharacter();
	if (!PlayerCharacter)
		return;

	const float Direction = Value.Get<float>();
	const FRotator Rotation = PlayerCharacter->GetController() ? PlayerCharacter->GetController()->GetControlRotation() : FRotator::ZeroRotator;
	const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);
	const FVector Right = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	if (Direction != 0.0f)
	{
		// Update current movement input for other systems (like dodge system)
		SetCurrentMovementInputAxis("Y", Direction);
		SetHasMovementInput(true);

		// Only add movement input if not dodging
		if (PlayerCharacter->DodgeSystem && !PlayerCharacter->DodgeSystem->IsDodging())
			PlayerCharacter->AddMovementInput(Right, Direction);
	}
}

void UMovementInputComponent::Look(const FInputActionValue& Value)
{
	APlayerCharacter* PlayerCharacter = GetValidPlayerCharacter();
	if (!PlayerCharacter)
		return;

	const FVector2D LookAxis = Value.Get<FVector2D>();
	if (PlayerCharacter->GetController())
	{
		float YawInput = LookAxis.X * MouseSensitivity;
		float PitchInput = LookAxis.Y * MouseSensitivity;

		// Apply Y-axis inversion if enabled
		if (bInvertYAxis)
			PitchInput *= -1.0f;

		PlayerCharacter->AddControllerYawInput(YawInput);
		PlayerCharacter->AddControllerPitchInput(PitchInput);
	}
}

void UMovementInputComponent::OnMovementInputCompleted(const FString& Axis)
{
	SetCurrentMovementInputAxis(Axis, 0.0f);
}

void UMovementInputComponent::UpdateMovementVelocity()
{
	APlayerCharacter* PlayerCharacter = GetValidPlayerCharacter();
	if (!PlayerCharacter)
		return;

	UCharacterMovementComponent* Movement = PlayerCharacter->GetCharacterMovement();
	if (Movement)
	{
		const FVector2D Velocity2D = FVector2D(Movement->Velocity.X, Movement->Velocity.Y);
		MovementSpeed2D = Velocity2D.Size();

		// Reset movement input tracking if no velocity
		if (MovementSpeed2D <= 0.0f)
		{
			CurrentMovementInput = FVector::ZeroVector;
			bHasMovementInput = false;
		}
	}
}

void UMovementInputComponent::UpdateMaxWalkSpeed()
{
	APlayerCharacter* PlayerCharacter = GetValidPlayerCharacter();
	if (!PlayerCharacter)
		return;

	UCharacterMovementComponent* Movement = PlayerCharacter->GetCharacterMovement();
	if (!Movement)
		return;

	float MaxSpeed;
	
	// Check dodge system first (highest priority)
	if (PlayerCharacter->DodgeSystem && PlayerCharacter->DodgeSystem->IsDodging())
	{
		MaxSpeed = PlayerCharacter->DodgeSystem->GetDodgeSpeed();
	}
	// Then check crouch state
	else if (PlayerCharacter->CrouchSystem && PlayerCharacter->CrouchSystem->IsCrouched())
	{
		MaxSpeed = PlayerCharacter->CrouchSystem->GetCrouchSpeed();
	}
	// Then check sprint state
	else if (!PlayerCharacter->SprintInterrupted)
	{
		MaxSpeed = PlayerCharacter->RunSpeed;
	}
	// Default to walk speed
	else
	{
		MaxSpeed = PlayerCharacter->WalkSpeed;
	}
	
	Movement->MaxWalkSpeed = MaxSpeed;
}

void UMovementInputComponent::SetCurrentMovementInputAxis(const FString& Axis, float Value)
{
	if (Axis == "X")
		CurrentMovementInput.X = Value;
	else if (Axis == "Y")
		CurrentMovementInput.Y = Value;
	else if (Axis == "Z")
		CurrentMovementInput.Z = Value;
}

