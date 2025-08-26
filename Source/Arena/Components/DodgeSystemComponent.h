// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DodgeSystemComponent.generated.h"

class APlayerCharacter;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ARENA_API UDodgeSystemComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UDodgeSystemComponent();

	virtual void BeginPlay() override;

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

	void SetCanDodge(bool NewCanDodge) { bCanDodge = NewCanDodge; }
	void SetIsDodging(bool NewIsDodging) { bIsDodging = NewIsDodging; }
	void SetDodgeSpeed(float NewDodgeSpeed) { DodgeSpeed = NewDodgeSpeed; }
	void SetDodgeCooldown(float NewDodgeCooldown) { DodgeCooldown = NewDodgeCooldown; }

	const FVector& GetCurrentMovementInput() const { return CurrentMovementInput; }
	void SetCurrentMovementInput(const FVector& NewCurrentMovementInput) { CurrentMovementInput = NewCurrentMovementInput; }
	void SetCurrentMovementInputAxis(FString Axis, float Value);

	const FVector& GetDodgeDirection() const { return DodgeDirection; }
	void SetDodgeDirection(const FVector& NewDodgeDirection) { DodgeDirection = NewDodgeDirection; }

	bool HasMovementInput() const { return bHasMovementInput; }
	void SetHasMovementInput(bool NewHasMovementInput) { bHasMovementInput = NewHasMovementInput; }

	bool WasCrouchingPreDodge() const { return bWasCrouchingPreDodge; }
    void SetWasCrouchingPreDodge(bool NewWasCrouchingPreDodge) { bWasCrouchingPreDodge = NewWasCrouchingPreDodge; }

	/** Gets the owning PlayerCharacter */
	UFUNCTION(BlueprintCallable, Category = "Dodge System", meta = (ToolTip = "Returns the PlayerCharacter owner"))
	APlayerCharacter* GetPlayerCharacter() const { return GetValidPlayerCharacter(); }

protected:
	/** How long dodge action lasts */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dodge System", 
		meta = (ToolTip = "Dodge duration in seconds", ClampMin = "0.1", ClampMax = "2.0"))
	float DodgeDuration = 0.7f;

	/** Time between consecutive dodges */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dodge System", 
		meta = (ToolTip = "Cooldown to re-dodge after a dodge finish in seconds", ClampMin = "0.0", ClampMax = "1.0"))
	float DodgeCooldown = 0.2f;

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

private:
	/** Timer for dodge cooldown */
	FTimerHandle DodgeTimerHandle;

	/** Current normalized movement input */
	FVector CurrentMovementInput = FVector::ZeroVector;

	/** World space dodge direction */
	FVector DodgeDirection = FVector::ZeroVector;

	/** Whether player is providing input */
	bool bHasMovementInput = false;

	/** Crouch state before dodge started */
	bool bWasCrouchingPreDodge = false;

	/** Timer for initialization retry */
	FTimerHandle InitializationRetryTimerHandle;

	/** Cached PlayerCharacter reference */
	TObjectPtr<APlayerCharacter> OwnerPlayerCharacter;

	/** Gets valid PlayerCharacter with fallback */
	FORCEINLINE APlayerCharacter* GetValidPlayerCharacter() const;
};
