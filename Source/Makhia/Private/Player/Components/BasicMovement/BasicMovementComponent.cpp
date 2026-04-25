// Copyright (c) 2025 Luigi Pennisi. All rights reserved.

#include "Player/Components/BasicMovement/BasicMovementComponent.h"
#include "Player/MKHPlayerCharacter.h"
#include "Player/MovementStateMachine/MovementStateMachine.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"

// Sets default values for this component's properties
UBasicMovementComponent::UBasicMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	MouseSensitivity = 1.0f;
	bInvertYAxis = false;
	MovementSmoothing = 0.1f;
}


 // Called when the game starts
void UBasicMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerPlayerCharacter = Cast<AMKHPlayerCharacter>(GetOwner());

	// Validation for safety in development
	if (!OwnerPlayerCharacter)
	{
		UE_LOG(LogTemp, Error, TEXT("BasicMovementComponent: Owner is not a MKHPlayerCharacter! Owner class: %s"),
			GetOwner() ? *GetOwner()->GetClass()->GetName() : TEXT("NULL"));
	}
}

void UBasicMovementComponent::SetupInput(UEnhancedInputComponent* EnhancedInputComponent)
{
	if (!EnhancedInputComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("BasicMovementComponent: EnhancedInputComponent is null"));
		return;
	}

	// Check if input actions are set
	if (!MoveForwardAction)
	{
		UE_LOG(LogTemp, Warning, TEXT("BasicMovementComponent: MoveForwardAction is not set"));
		return;
	}
	if (!MoveRightAction)
	{
		UE_LOG(LogTemp, Warning, TEXT("BasicMovementComponent: MoveRightAction is not set"));
		return;
	}
	if (!LookAction)
	{
		UE_LOG(LogTemp, Warning, TEXT("BasicMovementComponent: LookAction is not set"));
		return;
	}

	// Bind movement input using lambdas
	EnhancedInputComponent->BindActionValueLambda(MoveForwardAction, ETriggerEvent::Triggered, 
		[this](const FInputActionValue& Value) {
			MoveForward(Value);
		});
	EnhancedInputComponent->BindActionValueLambda(MoveForwardAction, ETriggerEvent::Completed, 
		[this](const FInputActionValue& Value) {
			OnMovementInputCompleted("X");
		});
	EnhancedInputComponent->BindActionValueLambda(MoveRightAction, ETriggerEvent::Triggered, 
		[this](const FInputActionValue& Value) {
			MoveRight(Value);
		});
	EnhancedInputComponent->BindActionValueLambda(MoveRightAction, ETriggerEvent::Completed, 
		[this](const FInputActionValue& Value) {
			OnMovementInputCompleted("Y");
		});
	EnhancedInputComponent->BindActionValueLambda(LookAction, ETriggerEvent::Triggered, 
		[this](const FInputActionValue& Value) {
			Look(Value);
		});


}

void UBasicMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Update movement velocity tracking
	UpdateMovementVelocity();
}

void UBasicMovementComponent::MoveForward(const FInputActionValue& Value)
{
	AMKHPlayerCharacter* MKHPlayerCharacter = GetValidPlayerCharacter();
	if (!MKHPlayerCharacter)
		return;

	const float Direction = Value.Get<float>();
	const FRotator Rotation = MKHPlayerCharacter->GetController() ? MKHPlayerCharacter->GetController()->GetControlRotation() : FRotator::ZeroRotator;
	const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);
	const FVector Forward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

	if (Direction != 0.0f)
	{
		// Update current movement input for other systems (like dodge system)
		SetCurrentMovementInputAxis("X", Direction);
		SetHasMovementInput(true);

		// Only add movement input if not dodging
		if (MKHPlayerCharacter->MovementStateMachine)
		{
			EMovementStateValue CurrentState = MKHPlayerCharacter->GetMovementStateMachine()->GetCurrentState();
			if (CurrentState != EMovementStateValue::Dodging)
				MKHPlayerCharacter->AddMovementInput(Forward, Direction);
		}
	}
}

void UBasicMovementComponent::MoveRight(const FInputActionValue& Value)
{
	AMKHPlayerCharacter* MKHPlayerCharacter = GetValidPlayerCharacter();
	if (!MKHPlayerCharacter)
		return;

	const float Direction = Value.Get<float>();
	const FRotator Rotation = MKHPlayerCharacter->GetController() ? MKHPlayerCharacter->GetController()->GetControlRotation() : FRotator::ZeroRotator;
	const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);
	const FVector Right = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	if (Direction != 0.0f)
	{
		// Update current movement input for other systems (like dodge system)
		SetCurrentMovementInputAxis("Y", Direction);
		SetHasMovementInput(true);

		// Only add movement input if not dodging
		if (MKHPlayerCharacter->MovementStateMachine)
		{
			EMovementStateValue CurrentState = MKHPlayerCharacter->GetMovementStateMachine()->GetCurrentState();
			if (CurrentState != EMovementStateValue::Dodging)
				MKHPlayerCharacter->AddMovementInput(Right, Direction);
		}
	}
}

void UBasicMovementComponent::Look(const FInputActionValue& Value)
{
	AMKHPlayerCharacter* MKHPlayerCharacter = GetValidPlayerCharacter();
	if (!MKHPlayerCharacter)
		return;

	const FVector2D LookAxis = Value.Get<FVector2D>();
	if (MKHPlayerCharacter->GetController())
	{
		float YawInput = LookAxis.X * MouseSensitivity;
		float PitchInput = LookAxis.Y * MouseSensitivity;

		// Apply Y-axis inversion if enabled
		if (bInvertYAxis)
			PitchInput *= -1.0f;

		MKHPlayerCharacter->AddControllerYawInput(YawInput);
		MKHPlayerCharacter->AddControllerPitchInput(PitchInput);
	}
}

void UBasicMovementComponent::OnMovementInputCompleted(const FString& Axis)
{
	SetCurrentMovementInputAxis(Axis, 0.0f);
}

void UBasicMovementComponent::UpdateMovementVelocity()
{
	AMKHPlayerCharacter* MKHPlayerCharacter = GetValidPlayerCharacter();
	if (!MKHPlayerCharacter)
		return;

	UCharacterMovementComponent* Movement = MKHPlayerCharacter->GetCharacterMovement();
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

void UBasicMovementComponent::SetCurrentMovementInputAxis(const FString& Axis, float Value)
{
	if (Axis == "X")
		CurrentMovementInput.X = Value;
	else if (Axis == "Y")
		CurrentMovementInput.Y = Value;
	else if (Axis == "Z")
		CurrentMovementInput.Z = Value;
}

AMKHPlayerCharacter *UBasicMovementComponent::GetValidPlayerCharacter() const
{
	if (LIKELY(OwnerPlayerCharacter != nullptr))
	{
		return OwnerPlayerCharacter.Get();
	}

	// Fallback: re-cache if necessary (should be very rare)
	return Cast<AMKHPlayerCharacter>(GetOwner());
}