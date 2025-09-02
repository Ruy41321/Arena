// Copyright (c) 2025 Luigi Pennisi. All rights reserved.

#include "SprintSystemComponent.h"
#include "../../Player/PlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "Net/UnrealNetwork.h"

USprintSystemComponent::USprintSystemComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bSprintInterrupted = true;
	bIsSprinting = false;

	SetIsReplicatedByDefault(true);
}

void USprintSystemComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(USprintSystemComponent, bIsSprinting);
}

void USprintSystemComponent::BeginPlay()
{
	Super::BeginPlay();
	
	OwnerPlayerCharacter = Cast<APlayerCharacter>(GetOwner());
	
	if (!OwnerPlayerCharacter)
	{
		UE_LOG(LogTemp, Error, TEXT("SprintSystemComponent: Owner is not a PlayerCharacter! Owner class: %s"),
			GetOwner() ? *GetOwner()->GetClass()->GetName() : TEXT("NULL"));
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


}

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
	}
	else
	{
		bSprintInterrupted = true;
		bIsSprinting = false;
	}
}

