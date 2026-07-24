// Copyright (c) 2025 Luigi Pennisi. All rights reserved.
#include "Player/Components/Dodge/DodgeSystemComponent.h"
#include "Player/MovementStateMachine/MovementStateMachine.h"
#include "Player/Components/Crouch/CrouchSystemComponent.h"
#include "Player/Components/BasicMovement/BasicMovementComponent.h"
#include "Player/MKHPlayerCharacter.h"
#include "InputActionValue.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/CharacterMovementComponent.h"

// ============================================================
// Lifecycle
// ============================================================

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
	
	OwnerPlayerCharacter = Cast<AMKHPlayerCharacter>(GetOwner());
	
	if (!IsValid(OwnerPlayerCharacter))
	{
		UE_LOG(LogTemp, Error, TEXT("DodgeSystemComponent: Owner is not a MKHPlayerCharacter! Owner class: %s"),
			GetOwner() ? *GetOwner()->GetClass()->GetName() : TEXT("NULL"));
	}
}

void UDodgeSystemComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AMKHPlayerCharacter* MKHPlayerCharacter = GetValidPlayerCharacter();
	if (!IsValid(MKHPlayerCharacter) || !IsValid(MKHPlayerCharacter->GetCharacterMovement()))
	{
		return;
	}

	// Apply movement input only for locally controlled players during dodge maneuver.
	// Remote clients rely on the standard CharacterMovementComponent networking pipeline.
	if (IsDodging() && !GetDodgeDirection().IsZero() && MKHPlayerCharacter->IsLocallyControlled())
	{
		UpdateDodgeDirection();
		MKHPlayerCharacter->AddMovementInput(GetDodgeDirection(), 1.0f);
	}
}

// ============================================================
// Public Interface
// ============================================================

bool UDodgeSystemComponent::StartDodge()
{
	AMKHPlayerCharacter* MKHPlayerCharacter = GetValidPlayerCharacter();
	if (!IsValid(MKHPlayerCharacter))
	{
		return false;
	}

	if (IsValid(MKHPlayerCharacter->GetMovementStateMachine()))
	{
		const EMovementStateValue CurrentState = MKHPlayerCharacter->GetMovementStateMachine()->GetCurrentState();

		if (!IsInDodgeableState(CurrentState))
		{
			return false;
		}

		// Cache crouching state and force a crouch transition if needed before dodging
		bWasCrouchingPreDodge = (CurrentState == EMovementStateValue::CrouchingIdle || CurrentState == EMovementStateValue::CrouchingMoving);
		if (IsValid(MKHPlayerCharacter->CrouchSystem) && !bWasCrouchingPreDodge)
		{
			MKHPlayerCharacter->CrouchSystem->CrouchPressed(FInputActionValue());
		}
	}

	bIsDodging = true;
	MKHPlayerCharacter->GetMovementStateMachine()->TransitionToState(EMovementStateValue::Dodging);

	// Determine dodge direction at startup
	if (!UpdateDodgeDirection())
	{
		DodgeDirection = MKHPlayerCharacter->GetActorForwardVector();
	}

	InitialDodgeDirection = DodgeDirection;

	return true;
}

void UDodgeSystemComponent::ResetDodge()
{
	AMKHPlayerCharacter* MKHPlayerCharacter = GetValidPlayerCharacter();
	if (!IsValid(MKHPlayerCharacter) || !bIsDodging)
	{
		return;
	}

	bIsDodging = false;
	DodgeDirection = FVector::ZeroVector;
	InitialDodgeDirection = FVector::ZeroVector;

	// Restore original crouch state if possible
	if (!bWasCrouchingPreDodge && IsValid(MKHPlayerCharacter->CrouchSystem) && MKHPlayerCharacter->CrouchSystem->CanUncrouchSafely())
	{
		MKHPlayerCharacter->CrouchSystem->CrouchPressed(FInputActionValue());
		bWasCrouchingPreDodge = false;
	}
}

bool UDodgeSystemComponent::UpdateDodgeDirection()
{
	bool bHasInput = false;
	FVector MovementInput = FVector::ZeroVector;

	if (!TryFetchMovementInput(bHasInput, MovementInput))
	{
		return false;
	}

	// If we are starting a dodge or have no direction yet, handle initial calculation
	if (!bIsDodging || InitialDodgeDirection.IsZero())
	{
		return HandleInitialDodgeDirection(bHasInput, MovementInput);
	}

	// Blend with current input if we are already dodging
	UpdateBlendedDodgeDirection(bHasInput, MovementInput);
	
	return true;
}

bool UDodgeSystemComponent::IsInDodgeableState(EMovementStateValue CurrentState) const
{
	switch (CurrentState)
	{
		case EMovementStateValue::Idle:
		case EMovementStateValue::CrouchingIdle:
		case EMovementStateValue::CrouchingMoving:
		case EMovementStateValue::Walking:
		case EMovementStateValue::Sprinting:
		case EMovementStateValue::Blocking:
		case EMovementStateValue::Attacking:
		case EMovementStateValue::Dodging:
			return true;
		default:
			return false;
	}
}

// ============================================================
// Protected / Internal Logic
// ============================================================

AMKHPlayerCharacter* UDodgeSystemComponent::GetValidPlayerCharacter() const
{
	if (LIKELY(IsValid(OwnerPlayerCharacter)))
	{
		return OwnerPlayerCharacter.Get();
	}

	return Cast<AMKHPlayerCharacter>(GetOwner());
}

void UDodgeSystemComponent::OnRep_bWasCrouchingPreDodge()
{
	AMKHPlayerCharacter* MKHPlayerCharacter = GetValidPlayerCharacter();
	if (IsValid(MKHPlayerCharacter) && IsValid(MKHPlayerCharacter->CrouchSystem) && !bWasCrouchingPreDodge)
	{
		MKHPlayerCharacter->CrouchSystem->CrouchPressed(FInputActionValue());
	}
}

bool UDodgeSystemComponent::TryFetchMovementInput(bool& bOutHasInput, FVector& OutMovementInput) const
{
	AMKHPlayerCharacter* MKHPlayerCharacter = GetValidPlayerCharacter();
	if (!IsValid(MKHPlayerCharacter))
	{
		return false;
	}

	// Detect input source: Enhanced Input for local player, Acceleration for remote clients
	if (MKHPlayerCharacter->IsLocallyControlled())
	{
		if (IsValid(MKHPlayerCharacter->BasicMovementSystem))
		{
			bOutHasInput = MKHPlayerCharacter->BasicMovementSystem->HasMovementInput();
			OutMovementInput = MKHPlayerCharacter->BasicMovementSystem->GetCurrentMovementInput();
			return true;
		}
	}
	else
	{
		if (UCharacterMovementComponent* MoveComp = MKHPlayerCharacter->GetCharacterMovement())
		{
			OutMovementInput = MoveComp->GetCurrentAcceleration().GetSafeNormal();
			bOutHasInput = !OutMovementInput.IsNearlyZero();
			return true;
		}
	}

	return false;
}

FVector UDodgeSystemComponent::CalculateLocalInputDirection(const FVector& MovementInput) const
{
	AMKHPlayerCharacter* MKHPlayerCharacter = GetValidPlayerCharacter();
	if (!IsValid(MKHPlayerCharacter) || !IsValid(MKHPlayerCharacter->Controller))
	{
		return FVector::ZeroVector;
	}

	const FRotator Rotation = MKHPlayerCharacter->Controller->GetControlRotation();
	const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);
	
	const FVector Forward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector Right = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	return (Forward * MovementInput.X + Right * MovementInput.Y).GetSafeNormal();
}

bool UDodgeSystemComponent::HandleInitialDodgeDirection(bool bHasInput, const FVector& MovementInput)
{
	if (!(bHasInput && !MovementInput.IsNearlyZero()))
	{
		return false;
	}

	AMKHPlayerCharacter* MKHPlayerCharacter = GetValidPlayerCharacter();
	if (!IsValid(MKHPlayerCharacter))
	{
		return false;
	}

	if (MKHPlayerCharacter->IsLocallyControlled())
	{
		DodgeDirection = CalculateLocalInputDirection(MovementInput);
	}
	else
	{
		DodgeDirection = MovementInput;
	}

	return true;
}

void UDodgeSystemComponent::UpdateBlendedDodgeDirection(bool bHasInput, const FVector& MovementInput)
{
	FVector CurrentInputDirection = FVector::ZeroVector;
	
	if (bHasInput && !MovementInput.IsNearlyZero())
	{
		AMKHPlayerCharacter* MKHPlayerCharacter = GetValidPlayerCharacter();
		if (IsValid(MKHPlayerCharacter))
		{
			if (MKHPlayerCharacter->IsLocallyControlled())
			{
				CurrentInputDirection = CalculateLocalInputDirection(MovementInput);
			}
			else
			{
				CurrentInputDirection = MovementInput;
			}
		}
	}
	
	if (CurrentInputDirection.IsZero())
	{
		DodgeDirection = InitialDodgeDirection;
	}
	else
	{
		DodgeDirection = FMath::Lerp(InitialDodgeDirection, CurrentInputDirection, InputInfluenceFactor).GetSafeNormal();
	}
}
