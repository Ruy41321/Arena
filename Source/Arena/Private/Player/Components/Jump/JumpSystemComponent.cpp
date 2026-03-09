// Copyright (c) 2025 Luigi Pennisi. All rights reserved.

#include "Player/Components/Jump/JumpSystemComponent.h"
#include "Player/MovementStateMachine/MovementStateMachine.h"
#include "Player/Components/Crouch/CrouchSystemComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "InputActionValue.h"
#include "Player/PlayerCharacter.h"
#include "GameFramework/Character.h"
#include "EnhancedInputComponent.h"
#include "Net/UnrealNetwork.h"

UJumpSystemComponent::UJumpSystemComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	SetIsReplicatedByDefault(true);
}

void UJumpSystemComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UJumpSystemComponent, bIsLanding);
}

void UJumpSystemComponent::BeginPlay()
{
	Super::BeginPlay();
	
	OwnerPlayerCharacter = Cast<APlayerCharacter>(GetOwner());
	
	// Validation for safety in development
	if (!OwnerPlayerCharacter)
	{
		UE_LOG(LogTemp, Error, TEXT("JumpSystemComponent: Owner is not a PlayerCharacter! Owner class: %s"),
			GetOwner() ? *GetOwner()->GetClass()->GetName() : TEXT("NULL"));
	}
}

void UJumpSystemComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Jump system doesn't need constant tick updates
	// All logic is event-driven through input and landing callbacks
}

void UJumpSystemComponent::JumpPressed(const FInputActionValue& Value)
{
	APlayerCharacter* PlayerCharacter = GetValidPlayerCharacter();
	if (!PlayerCharacter)
		return;

	if (PlayerCharacter->GetMovementStateMachine())
	{
		EMovementStateValue CurrentState = PlayerCharacter->GetMovementStateMachine()->GetCurrentState();
				
		if (bIsLanding || CurrentState == EMovementStateValue::Dodging)
			return;

		// If crouched, uncrouch instead of jumping
		if ((CurrentState == EMovementStateValue::CrouchingIdle || CurrentState == EMovementStateValue::CrouchingMoving) 
			&& PlayerCharacter->CrouchSystem)
		{
			PlayerCharacter->CrouchSystem->CrouchPressed(Value);
			return;
		}
	}
	// Perform the jump
	PlayerCharacter->Jump();
}

void UJumpSystemComponent::OnLanded(const FHitResult& Hit)
{
	APlayerCharacter* PlayerCharacter = GetValidPlayerCharacter();
	if (!PlayerCharacter)
		return;

	bIsLanding = true;
	
	// Set a timer to reset landing state
	GetWorld()->GetTimerManager().SetTimer(LandingTimerHandle, this, &UJumpSystemComponent::ResetLanding, LandingResetTime, false);
	
	UE_LOG(LogTemp, Verbose, TEXT("JumpSystem: Character landed, setting landing state for %f seconds"), LandingResetTime);
}

void UJumpSystemComponent::ResetLanding()
{
	bIsLanding = false;
}

void UJumpSystemComponent::SetupInput(UEnhancedInputComponent* EnhancedInputComponent)
{
	if (!EnhancedInputComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("JumpSystemComponent: EnhancedInputComponent is null"));
		return;
	}

	if (!JumpPressedAction)
	{
		UE_LOG(LogTemp, Warning, TEXT("JumpSystemComponent: JumpPressedAction is not set"));
		return;
	}

	APlayerCharacter* PlayerCharacter = GetValidPlayerCharacter();
	if (!PlayerCharacter)
	{
		UE_LOG(LogTemp, Warning, TEXT("JumpSystemComponent: PlayerCharacter is not available for input binding"));
		return;
	}

	// Bind jump input
	EnhancedInputComponent->BindActionValueLambda(JumpPressedAction, ETriggerEvent::Started, 
		[this](const FInputActionValue& Value) {
			JumpPressed(Value);
		});
	EnhancedInputComponent->BindAction(JumpPressedAction, ETriggerEvent::Completed, PlayerCharacter, &ACharacter::StopJumping);


}

APlayerCharacter* UJumpSystemComponent::GetValidPlayerCharacter() const
{
	if (LIKELY(OwnerPlayerCharacter != nullptr))
	{
		return OwnerPlayerCharacter.Get();
	}

	// Fallback: re-cache if necessary (should be very rare)
	return Cast<APlayerCharacter>(GetOwner());
}