// Fill out your copyright notice in the Description page of Project Settings.

#include "DodgeSystemComponent.h"
#include "../../Player/PlayerCharacter.h"
#include "EnhancedInputComponent.h"

// Sets default values for this component's properties
UDodgeSystemComponent::UDodgeSystemComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
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
	
	UE_LOG(LogTemp, Log, TEXT("DodgeSystemComponent: Input bindings set up successfully"));
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

	if (!bCanDodge || !PlayerCharacter || 
		(PlayerCharacter->JumpSystem && PlayerCharacter->JumpSystem->IsLanding()) || 
		PlayerCharacter->GetCharacterMovement()->IsFalling())
		return;

	// Use CrouchSystem instead of direct PlayerCharacter calls
	bWasCrouchingPreDodge = PlayerCharacter->CrouchSystem ? PlayerCharacter->CrouchSystem->IsCrouched() : false;
	if (PlayerCharacter->CrouchSystem)
	{
		if (!PlayerCharacter->CrouchSystem->IsCrouched())
			PlayerCharacter->CrouchSystem->CrouchPressed(FInputActionValue());
		else
			bWasCrouchingPreDodge = true;
	}

	bIsDodging = true;
	// Use BasicMovementComponent for speed updates
	if (PlayerCharacter->BasicMovementSystem)
		PlayerCharacter->BasicMovementSystem->UpdateMaxWalkSpeed();

	// If no movement input, set dodge direction to forward
	if (!UpdateDodgeDirection())
	{
		DodgeDirection = PlayerCharacter->GetActorForwardVector();
		UE_LOG(LogTemp, Warning, TEXT("DodgeSystem: No movement input, using forward direction: %s"), *DodgeDirection.ToString());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("DodgeSystem: Using movement input direction: %s"), *DodgeDirection.ToString());
	}
	
	// Store the initial dodge direction for blending
	InitialDodgeDirection = DodgeDirection;
	
	bCanDodge = false;
	// Set a timer to reset dodge after a short duration as security in case animation notify fails
	GetWorld()->GetTimerManager().SetTimer(DodgeTimerHandle, this, &UDodgeSystemComponent::ResetDodge, DodgeDuration, false);
	
	UE_LOG(LogTemp, Warning, TEXT("DodgeSystem: Dodge started! Direction: %s, Speed: %f"), *DodgeDirection.ToString(), DodgeSpeed);
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
	if (PlayerCharacter->CrouchSystem)
	{
		if (PlayerCharacter->CrouchSystem->CanUncrouchSafely() && !bWasCrouchingPreDodge)
			PlayerCharacter->CrouchSystem->CrouchPressed(FInputActionValue());
	}
	bWasCrouchingPreDodge = false;

	// Reset speed based on player state - use BasicMovementComponent
	if (PlayerCharacter->BasicMovementSystem)
		PlayerCharacter->BasicMovementSystem->UpdateMaxWalkSpeed();

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
		// No BasicMovementComponent available - can't proceed
		UE_LOG(LogTemp, Warning, TEXT("DodgeSystem: BasicMovementComponent not available!"));
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