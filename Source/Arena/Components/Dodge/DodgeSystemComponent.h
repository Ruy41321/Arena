// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InputActionValue.h"
#include "DodgeSystemComponent.generated.h"

class APlayerCharacter;
class UInputAction;
class UEnhancedInputComponent;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ARENA_API UDodgeSystemComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UDodgeSystemComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Sets up input bindings for this component */
	UFUNCTION(BlueprintCallable, Category = "Dodge System", meta = (ToolTip = "Sets up dodge input bindings"))
	void SetupInput(UEnhancedInputComponent* EnhancedInputComponent);

	/** Starts dodge in movement direction or forward if stationary */
	UFUNCTION(BlueprintCallable, Category = "Dodge System", meta = (ToolTip = "Initiates dodge maneuver"))
	void StartDodge();

	/** Ends dodge and starts cooldown timer */
	UFUNCTION(BlueprintCallable, Category = "Dodge System", meta = (ToolTip = "Resets dodge state and begins cooldown"))
	void ResetDodge();

	/** Updates dodge direction based on player input */
	UFUNCTION(BlueprintCallable, Category = "Dodge System", meta = (ToolTip = "Recalculates dodge direction from input"))
	bool UpdateDodgeDirection();

	/** Returns true if dodge is available */
	UFUNCTION(BlueprintPure, Category = "Dodge System", meta = (ToolTip = "Check if dodge is off cooldown"))
	bool CanDodge() const { return bCanDodge; }

	/** Returns true if currently dodging */
	UFUNCTION(BlueprintPure, Category = "Dodge System", meta = (ToolTip = "Check if player is dodging"))
	bool IsDodging() const { return bIsDodging; }

	/** Gets dodge movement speed */
	UFUNCTION(BlueprintPure, Category = "Dodge System", meta = (ToolTip = "Returns dodge speed in cm/s"))
	float GetDodgeSpeed() const { return DodgeSpeed; }

	/** Gets cooldown duration */
	UFUNCTION(BlueprintPure, Category = "Dodge System", meta = (ToolTip = "Returns cooldown time in seconds"))
	float GetDodgeCooldown() const { return DodgeCooldown; }

	/** Gets input influence factor */
	UFUNCTION(BlueprintPure, Category = "Dodge System", meta = (ToolTip = "Returns how much input affects dodge direction"))
	float GetInputInfluenceFactor() const { return InputInfluenceFactor; }

	void SetCanDodge(bool NewCanDodge) { bCanDodge = NewCanDodge; }
	void SetIsDodging(bool NewIsDodging) { bIsDodging = NewIsDodging; }
	void SetDodgeSpeed(float NewDodgeSpeed) { DodgeSpeed = NewDodgeSpeed; }
	void SetDodgeCooldown(float NewDodgeCooldown) { DodgeCooldown = NewDodgeCooldown; }
	void SetInputInfluenceFactor(float NewInputInfluenceFactor) { InputInfluenceFactor = FMath::Clamp(NewInputInfluenceFactor, 0.0f, 1.0f); }

	const FVector& GetDodgeDirection() const { return DodgeDirection; }
	void SetDodgeDirection(const FVector& NewDodgeDirection) { DodgeDirection = NewDodgeDirection; }

	const FVector& GetInitialDodgeDirection() const { return InitialDodgeDirection; }

	bool WasCrouchingPreDodge() const { return bWasCrouchingPreDodge; }
    void SetWasCrouchingPreDodge(bool NewWasCrouchingPreDodge) { bWasCrouchingPreDodge = NewWasCrouchingPreDodge; }

	/** Gets the owning PlayerCharacter */
	UFUNCTION(BlueprintCallable, Category = "Dodge System", meta = (ToolTip = "Returns the PlayerCharacter owner"))
	APlayerCharacter* GetPlayerCharacter() const { return GetValidPlayerCharacter(); }

	/** Gets valid PlayerCharacter with fallback */
	FORCEINLINE APlayerCharacter* GetValidPlayerCharacter() const
	{
		// Simple null check with cached reference for optimal performance
		if (LIKELY(OwnerPlayerCharacter != nullptr))
		{
			return OwnerPlayerCharacter.Get();
		}
		
		// Fallback: re-cache if necessary (should be very rare)
		return Cast<APlayerCharacter>(GetOwner());
	}

protected:
	/** How long dodge action lasts */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dodge System", 
		meta = (ToolTip = "Dodge duration in seconds", ClampMin = "0.1", ClampMax = "2.0"))
	float DodgeDuration = 0.7f;

	/** Time between consecutive dodges */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dodge System", 
		meta = (ToolTip = "Cooldown to re-dodge after a dodge finish in seconds", ClampMin = "0.0", ClampMax = "1.0"))
	float DodgeCooldown = 0.2f;

	/** How much user input influences dodge direction during dodge execution */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dodge System", 
		meta = (ToolTip = "Controls how much player input affects dodge direction. 0 = no influence (locked direction), 1 = full influence (current behavior)", ClampMin = "0.0", ClampMax = "1.0"))
	float InputInfluenceFactor = 0.49f;

	/** Whether dodge is available */
	UPROPERTY(BlueprintReadOnly, Category = "Dodge System", 
		meta = (ToolTip = "True if dodge is off cooldown"))
	bool bCanDodge = true;

	/** Whether currently executing dodge */
	UPROPERTY(BlueprintReadOnly, Category = "Dodge System", 
		meta = (ToolTip = "True while dodging"))
	bool bIsDodging = false;

	/** Movement speed during dodge */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dodge System", 
		meta = (ToolTip = "Dodge speed in cm/s", ClampMin = "100.0", ClampMax = "1000.0"))
	float DodgeSpeed = 650.0f;

public:
	/** Input action for dodge */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> DodgeAction;

private:
	/** Timer for dodge cooldown */
	FTimerHandle DodgeTimerHandle;

	/** World space dodge direction */
	FVector DodgeDirection = FVector::ZeroVector;

	/** Initial dodge direction when dodge started (used as base for blending) */
	FVector InitialDodgeDirection = FVector::ZeroVector;

	/** Crouch state before dodge started */
	bool bWasCrouchingPreDodge = false;

	/** Timer for initialization retry */
	FTimerHandle InitializationRetryTimerHandle;

	/** Cached PlayerCharacter reference */
	TObjectPtr<APlayerCharacter> OwnerPlayerCharacter;
};