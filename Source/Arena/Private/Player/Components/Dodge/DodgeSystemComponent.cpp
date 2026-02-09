// Copyright (c) 2025 Luigi Pennisi. All rights reserved.

#include "Player/Components/Dodge/DodgeSystemComponent.h"
#include "Player/MovementStateMachine/MovementStateMachine.h"
#include "Player/Components/Crouch/CrouchSystemComponent.h"
#include "Player/Components/BasicMovement/BasicMovementComponent.h"
#include "Player/PlayerCharacter.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "Net/UnrealNetwork.h"

UDodgeSystemComponent::UDodgeSystemComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

void UDodgeSystemComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UDodgeSystemComponent, bIsDodging);
}

void UDodgeSystemComponent::BeginPlay()
{
	Super::BeginPlay();
	
	OwnerPlayerCharacter = Cast<APlayerCharacter>(GetOwner());
	
	if (!OwnerPlayerCharacter)
	{
		UE_LOG(LogTemp, Error, TEXT("DodgeSystemComponent: Owner is not a PlayerCharacter! Owner class: %s"),
			GetOwner() ? *GetOwner()->GetClass()->GetName() : TEXT("NULL"));
	}
}

void UDodgeSystemComponent::SetupInput(UEnhancedInputComponent* EnhancedInputComponent)
{
	if (!EnhancedInputComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("DodgeSystemComponent: EnhancedInputComponent is null"));
		return;
	}

	if (!DodgeAction)
	{
		UE_LOG(LogTemp, Warning, TEXT("DodgeSystemComponent: DodgeAction is not set"));
		return;
	}

	// Bind dodge input
	EnhancedInputComponent->BindActionValueLambda(DodgeAction, ETriggerEvent::Started,
		[this](const FInputActionValue& Value) {
			StartDodge();
		});
	

}

void UDodgeSystemComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	APlayerCharacter* PlayerCharacter = GetValidPlayerCharacter();
	if (!PlayerCharacter || !PlayerCharacter->GetCharacterMovement())
		return;

	// Movement input tracking is now handled entirely by BasicMovementComponent
	// No need to duplicate velocity tracking here

	if (IsDodging() && !GetDodgeDirection().IsZero())
	{
		UpdateDodgeDirection();
		PlayerCharacter->AddMovementInput(GetDodgeDirection(), 1.0f);
	}
}

void UDodgeSystemComponent::StartDodge()
{
	APlayerCharacter* PlayerCharacter = GetValidPlayerCharacter();
	if (!PlayerCharacter)
		return;

	if (!PlayerCharacter->HasAuthority())
	{
		ServerStartDodge();
	}

	if (PlayerCharacter->GetMovementStateMachine())
	{
		EMovementStateValue CurrentState = PlayerCharacter->GetMovementStateMachine()->GetCurrentState();

		if (!bCanDodge || !IsInDodgeableState(CurrentState))
			return;

		// Use CrouchSystem instead of direct PlayerCharacter calls
		bWasCrouchingPreDodge = (CurrentState == EMovementStateValue::CrouchingIdle || CurrentState == EMovementStateValue::CrouchingMoving);
		if (PlayerCharacter->CrouchSystem && !bWasCrouchingPreDodge)
		{
			PlayerCharacter->CrouchSystem->CrouchPressed(FInputActionValue());
		}
	}
	bIsDodging = true;
	// Speed is now automatically set by the Movement State Machine when entering Dodging state

	// If no movement input, set dodge direction to forward
	if (!UpdateDodgeDirection())
	{
		DodgeDirection = PlayerCharacter->GetActorForwardVector();
	}

	// Store the initial dodge direction for blending
	InitialDodgeDirection = DodgeDirection;

	bCanDodge = false;
	// Set a timer to reset dodge after a short duration as security in case animation notify fails
	GetWorld()->GetTimerManager().SetTimer(DodgeTimerHandle, this, &UDodgeSystemComponent::ResetDodge, DodgeDuration, false);
}

void UDodgeSystemComponent::ServerStartDodge_Implementation()
{
	StartDodge();
}

void UDodgeSystemComponent::ResetDodge()
{
	APlayerCharacter* PlayerCharacter = GetValidPlayerCharacter();
	if (!PlayerCharacter || !bIsDodging)
		return;

	bIsDodging = false;

	// Clear dodge direction and initial direction
	DodgeDirection = FVector::ZeroVector;
	InitialDodgeDirection = FVector::ZeroVector;

	// Uncrouch if it was so and its possible - use CrouchSystem
	if (!bWasCrouchingPreDodge && PlayerCharacter->CrouchSystem && PlayerCharacter->CrouchSystem->CanUncrouchSafely())
	{
		PlayerCharacter->CrouchSystem->CrouchPressed(FInputActionValue());
		bWasCrouchingPreDodge = false;
	}

	// Start cooldown timer to re-enable dodging
	GetWorld()->GetTimerManager().ClearTimer(DodgeTimerHandle);
	GetWorld()->GetTimerManager().SetTimer(DodgeTimerHandle, [this]() { bCanDodge = true; }, DodgeCooldown, false);
}

bool UDodgeSystemComponent::UpdateDodgeDirection()
{
	APlayerCharacter* PlayerCharacter = GetValidPlayerCharacter();
	if (!PlayerCharacter)
		return false;

	// Get movement input from BasicMovementComponent - this is now the only source of truth
	bool bHasInput = false;
	FVector MovementInput = FVector::ZeroVector;

	if (PlayerCharacter->BasicMovementSystem)
	{
		bHasInput = PlayerCharacter->BasicMovementSystem->HasMovementInput();
		MovementInput = PlayerCharacter->BasicMovementSystem->GetCurrentMovementInput();
	}
	else
	{
		return false;
	}

	// If we're not dodging or don't have an initial direction, use original behavior
	if (!bIsDodging || InitialDodgeDirection.IsZero())
	{
		if (!(bHasInput && !MovementInput.IsNearlyZero()))
			return false;

		// Player is moving, dodge in the current movement direction
		const FRotator Rotation = PlayerCharacter->Controller ? PlayerCharacter->Controller->GetControlRotation() : FRotator::ZeroRotator;
		const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);
		const FVector Forward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector Right = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		DodgeDirection = (Forward * MovementInput.X + Right * MovementInput.Y).GetSafeNormal();
		return true;
	}

	// We're dodging - blend initial direction with current input based on influence factor
	FVector CurrentInputDirection = FVector::ZeroVector;
	
	// Calculate current input direction if player is providing input
	if (bHasInput && !MovementInput.IsNearlyZero())
	{
		const FRotator Rotation = PlayerCharacter->Controller ? PlayerCharacter->Controller->GetControlRotation() : FRotator::ZeroRotator;
		const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);
		const FVector Forward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector Right = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		
		CurrentInputDirection = (Forward * MovementInput.X + Right * MovementInput.Y).GetSafeNormal();
	}
	
	// Blend between initial dodge direction and current input direction
	// InputInfluenceFactor = 0: stick to initial direction
	// InputInfluenceFactor = 1: follow current input completely
	if (CurrentInputDirection.IsZero())
	{
		// No input, use initial direction
		DodgeDirection = InitialDodgeDirection;
	}
	else
	{
		// Blend between initial and current input directions
		DodgeDirection = FMath::Lerp(InitialDodgeDirection, CurrentInputDirection, InputInfluenceFactor).GetSafeNormal();
	}
	
	return true;
}

bool UDodgeSystemComponent::IsInDodgeableState(EMovementStateValue CurrentState) const
{
	// Define states where dodging is not allowed
	switch (CurrentState)
	{
	case EMovementStateValue::Idle:
	case EMovementStateValue::CrouchingIdle:
	case EMovementStateValue::CrouchingMoving:
	case EMovementStateValue::Walking:
	case EMovementStateValue::Sprinting:
		return true; // Dodgeable states
	default:
		return false; // Not dodgeable states
	}
}

void UDodgeSystemComponent::OnRep_bWasCrouchingPreDodge()
{
	APlayerCharacter* PlayerCharacter = GetValidPlayerCharacter();
	if (!PlayerCharacter)
		return;
	if (PlayerCharacter->CrouchSystem && !bWasCrouchingPreDodge)
	{
		PlayerCharacter->CrouchSystem->CrouchPressed(FInputActionValue());
	}
}