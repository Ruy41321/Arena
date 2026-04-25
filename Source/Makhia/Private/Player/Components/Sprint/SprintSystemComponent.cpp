// Copyright (c) 2025 Luigi Pennisi. All rights reserved.

#include "Player/Components/Sprint/SprintSystemComponent.h"
#include "Player/MovementStateMachine/MovementStateMachine.h"
#include "Player/Components/Crouch/CrouchSystemComponent.h"
#include "Player/MKHPlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
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
	
	OwnerPlayerCharacter = Cast<AMKHPlayerCharacter>(GetOwner());
	
	if (!OwnerPlayerCharacter)
	{
		UE_LOG(LogTemp, Error, TEXT("SprintSystemComponent: Owner is not a MKHPlayerCharacter! Owner class: %s"),
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

void USprintSystemComponent::SprintPressed(const FInputActionValue& Value, const bool bOverrideSprint, const bool bValueToOverride)
{
	AMKHPlayerCharacter* MKHPlayerCharacter = GetValidPlayerCharacter();
	if (!MKHPlayerCharacter)
		return;

	const bool bSprintValue = bOverrideSprint ? bValueToOverride : Value.Get<bool>();

	if (!MKHPlayerCharacter->HasAuthority())
		ServerSprintPressed(Value, true, bSprintValue);

	if (bSprintValue)
	{
		bSprintInterrupted = false;
		bIsSprinting = true;
		// if (MKHPlayerCharacter->HasAuthority())
		// 	UE_LOG(LogTemp, Log, TEXT("Sprint started on server"));
		// Don't sprint if dodging
		if (MKHPlayerCharacter->GetMovementStateMachine())
		{
			EMovementStateValue CurrentState = MKHPlayerCharacter->GetMovementStateMachine()->GetCurrentState();
			if (CurrentState == EMovementStateValue::Dodging)
			{
				bIsSprinting = false;
				return;
			}

			// If crouched, try to uncrouch first
			if (CurrentState == EMovementStateValue::CrouchingIdle || CurrentState == EMovementStateValue::CrouchingMoving)
			{
				if (!MKHPlayerCharacter->CrouchSystem->CanUncrouchSafely())
				{
					bIsSprinting = false;
					return;
				}
				MKHPlayerCharacter->CrouchSystem->CrouchPressed(Value);
			}
		}
	}
	else
	{
		bSprintInterrupted = true;
		bIsSprinting = false;
	}
}

void USprintSystemComponent::ServerSprintPressed_Implementation(const FInputActionValue& Value, const bool bOverrideSprint, const bool bValueToOverride)
{
	SprintPressed(Value, bOverrideSprint, bValueToOverride);
}

AMKHPlayerCharacter* USprintSystemComponent::GetValidPlayerCharacter() const
{
	if (LIKELY(OwnerPlayerCharacter != nullptr))
	{
		return OwnerPlayerCharacter.Get();
	}

	// Fallback: re-cache if necessary (should be very rare)
	return Cast<AMKHPlayerCharacter>(GetOwner());
}
