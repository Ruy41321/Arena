// Copyright (c) 2025 Luigi Pennisi. All rights reserved.

#include "SprintSystemComponent.h"
#include "../../Player/PlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"

// Sets default values for this component's properties
USprintSystemComponent::USprintSystemComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// Initialize default values
	bSprintInterrupted = true;
	bIsSprinting = false;
}

// Called when the game starts
void USprintSystemComponent::BeginPlay()
{
	Super::BeginPlay();
	
	OwnerPlayerCharacter = Cast<APlayerCharacter>(GetOwner());
	
	// Validation for safety in development
	if (!OwnerPlayerCharacter)
	{
		UE_LOG(LogTemp, Error, TEXT("SprintSystemComponent: Owner is not a PlayerCharacter! Owner class: %s"),
			GetOwner() ? *GetOwner()->GetClass()->GetName() : TEXT("NULL"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("SprintSystemComponent: Successfully cached PlayerCharacter - %s at address %p"), 
			*OwnerPlayerCharacter->GetName(), static_cast<void*>(OwnerPlayerCharacter.Get()));
	}
}

void USprintSystemComponent::SetupInput(UEnhancedInputComponent* EnhancedInputComponent)
{
	if (!EnhancedInputComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("SprintSystemComponent: EnhancedInputComponent is null"));
		return;
	}

	if (!SprintAction)
	{
		UE_LOG(LogTemp, Warning, TEXT("SprintSystemComponent: SprintAction is not set"));
		return;
	}

	// Bind sprint input
	EnhancedInputComponent->BindActionValueLambda(SprintAction, ETriggerEvent::Started, 
		[this](const FInputActionValue& Value) {
			SprintPressed(Value);
		});
	EnhancedInputComponent->BindActionValueLambda(SprintAction, ETriggerEvent::Completed, 
		[this](const FInputActionValue& Value) {
			SprintPressed(Value);
		});

	UE_LOG(LogTemp, Log, TEXT("SprintSystemComponent: Input bindings set up successfully"));
}

// Called every frame
void USprintSystemComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void USprintSystemComponent::SprintPressed(const FInputActionValue& Value)
{
	APlayerCharacter* PlayerCharacter = GetValidPlayerCharacter();
	if (!PlayerCharacter)
		return;

	const bool bSprintValue = Value.Get<bool>();
	
	if (bSprintValue)
	{
		// Starting sprint
		bSprintInterrupted = false;
		bIsSprinting = true;
		
		// Don't sprint if dodging
		if (PlayerCharacter->MovementStateMachine)
		{
			EMovementState CurrentState = PlayerCharacter->MovementStateMachine->GetCurrentState();
			if (CurrentState == EMovementState::Dodging)
			{
				bIsSprinting = false;
				return;
			}

			// If crouched, try to uncrouch first
			if (CurrentState == EMovementState::CrouchingIdle || CurrentState == EMovementState::CrouchingMoving)
			{
				if (!PlayerCharacter->CrouchSystem->CanUncrouchSafely())
				{
					bIsSprinting = false;
					return;
				}
				PlayerCharacter->CrouchSystem->CrouchPressed(Value);
			}
		}
		UE_LOG(LogTemp, Verbose, TEXT("SprintSystem: Sprint started"));
	}
	else
	{
		// Stopping sprint
		bSprintInterrupted = true;
		bIsSprinting = false;
		
		UE_LOG(LogTemp, Verbose, TEXT("SprintSystem: Sprint stopped"));
	}
}

