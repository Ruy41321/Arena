// Fill out your copyright notice in the Description page of Project Settings.

#include "DodgeSystemComponent.h"
#include "../Player/PlayerCharacter.h"

// Sets default values for this component's properties
UDodgeSystemComponent::UDodgeSystemComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UDodgeSystemComponent::BeginPlay()
{
	Super::BeginPlay();
	
	OwnerPlayerCharacter = Cast<APlayerCharacter>(GetOwner());
	
	// Validation per sicurezza in development
	if (!OwnerPlayerCharacter)
	{
		UE_LOG(LogTemp, Error, TEXT("DodgeSystemComponent: Owner is not a PlayerCharacter! Owner class: %s"),
			GetOwner() ? *GetOwner()->GetClass()->GetName() : TEXT("NULL"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("DodgeSystemComponent: Successfully cached PlayerCharacter - %s at address %p"), 
			*OwnerPlayerCharacter->GetName(), static_cast<void*>(OwnerPlayerCharacter.Get()));
	}
}

// Helper function inline per performance massima
FORCEINLINE APlayerCharacter* UDodgeSystemComponent::GetValidPlayerCharacter() const
{
	// Doppia validazione per massima sicurezza con branch prediction ottimale
	if (LIKELY(OwnerPlayerCharacter && IsValid(OwnerPlayerCharacter)))
	{
		return OwnerPlayerCharacter;
	}
	
	// Fallback: re-cache se necessario (dovrebbe essere rarissimo)
	// Cast diretto senza modifica del membro const in fallback
	return Cast<APlayerCharacter>(GetOwner());
}

void UDodgeSystemComponent::StartDodge()
{
	APlayerCharacter* PlayerCharacter = GetValidPlayerCharacter();

	if (!bCanDodge || !PlayerCharacter || PlayerCharacter->IsLanding || PlayerCharacter->GetCharacterMovement()->IsFalling())
		return;

	bWasCrouchingPreDodge = PlayerCharacter->IsCrouched;
	if (!PlayerCharacter->IsCrouched)
		PlayerCharacter->CrouchPressed(FInputActionValue());
	else
		bWasCrouchingPreDodge = true;

	bIsDodging = true;
	PlayerCharacter->UpdateMaxWalkSpeed();

	// If no movement input, set dodge direction to forward
	if (!UpdateDodgeDirection())
		DodgeDirection = PlayerCharacter->GetActorForwardVector();
	bCanDodge = false;
	// Set a timer to reset dodge after a short duration as security in case animation notify fails
	GetWorld()->GetTimerManager().SetTimer(DodgeTimerHandle, this, &UDodgeSystemComponent::ResetDodge, DodgeDuration, false);
}

void UDodgeSystemComponent::ResetDodge()
{
	APlayerCharacter* PlayerCharacter = GetValidPlayerCharacter();
	if (!PlayerCharacter || !bIsDodging)
		return;

	bIsDodging = false;

	// Clear dodge direction
	DodgeDirection = FVector::ZeroVector;

	// Uncrouch if it was so and its possible
	if (PlayerCharacter->CanUncrouchSafely() && !bWasCrouchingPreDodge)
		PlayerCharacter->CrouchPressed(FInputActionValue());
	bWasCrouchingPreDodge = false;

	// Reset speed based on player state
	PlayerCharacter->UpdateMaxWalkSpeed();

	// Start cooldown timer to re-enable dodging
	GetWorld()->GetTimerManager().ClearTimer(DodgeTimerHandle);
	GetWorld()->GetTimerManager().SetTimer(DodgeTimerHandle, [this]() { bCanDodge = true; }, DodgeCooldown, false);
}

bool UDodgeSystemComponent::UpdateDodgeDirection()
{
	APlayerCharacter* PlayerCharacter = GetValidPlayerCharacter();
	if (!PlayerCharacter || !(bHasMovementInput && !CurrentMovementInput.IsNearlyZero()))
		return false;

	// Player is moving, dodge in the current movement direction
	const FRotator Rotation = PlayerCharacter->Controller ? PlayerCharacter->Controller->GetControlRotation() : FRotator::ZeroRotator;
	const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);
	const FVector Forward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector Right = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	DodgeDirection = (Forward * CurrentMovementInput.X + Right * CurrentMovementInput.Y).GetSafeNormal();
	return true;
}

void UDodgeSystemComponent::SetCurrentMovementInputAxis(FString Axis, float Value)
{
	if (Axis == "X")
		CurrentMovementInput.X = Value;
	else if (Axis == "Y")
		CurrentMovementInput.Y = Value;
	else if (Axis == "Z")
		CurrentMovementInput.Z = Value;
}
