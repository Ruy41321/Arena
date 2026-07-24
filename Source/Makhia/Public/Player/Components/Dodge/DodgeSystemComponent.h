// Copyright (c) 2025 Luigi Pennisi. All rights reserved.
#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DodgeSystemComponent.generated.h"
enum class EMovementStateValue : uint8;
class AMKHPlayerCharacter;
class UInputAction;
class UEnhancedInputComponent;
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class MAKHIA_API UDodgeSystemComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDodgeSystemComponent();

	// ============================================================
	// Lifecycle
	// ============================================================
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ============================================================
	// Public Interface
	// ============================================================
	/**
	 * Attempts to transition the movement state machine into the Dodge state.
	 * Validates current state before committing the transition.
	 *
	 * @return True if the transition was accepted, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "Movement|Dodge")
	bool StartDodge();

	/** 
	 * Ends the dodge state and resets directions.
	 * Attempts to uncrouch if needed.
	 */
	UFUNCTION(BlueprintCallable, Category = "Movement|Dodge")
	void ResetDodge();
	
	/**
	 * Updates the current dodge direction based on player input.
	 * @return True if the direction was updated, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "Movement|Dodge")
	bool UpdateDodgeDirection();

	/**
	 * Checks whether the provided state allows the character to dodge.
	 * @param CurrentState The state to evaluate.
	 * @return True if dodgeable, false otherwise.
	 */
	UFUNCTION(BlueprintPure, Category = "Movement|Dodge")
	bool IsInDodgeableState(EMovementStateValue CurrentState) const;

	/** Returns true if the player is currently executing a dodge maneuver. */
	UFUNCTION(BlueprintPure, Category = "Movement|Dodge")
	bool IsDodging() const { return bIsDodging; }

	/** Gets dodge movement speed in cm/s. */
	UFUNCTION(BlueprintPure, Category = "Movement|Dodge")
	float GetDodgeSpeed() const { return DodgeSpeed; }

	/** Gets how much input influences the dodge direction during the maneuver. */
	UFUNCTION(BlueprintPure, Category = "Movement|Dodge")
	float GetInputInfluenceFactor() const { return InputInfluenceFactor; }

	/** Gets the current world-space dodge direction. */
	UFUNCTION(BlueprintPure, Category = "Movement|Dodge")
	const FVector& GetDodgeDirection() const { return DodgeDirection; }

	/** Gets the initial dodge direction when the dodge originally started. */
	UFUNCTION(BlueprintPure, Category = "Movement|Dodge")
	const FVector& GetInitialDodgeDirection() const { return InitialDodgeDirection; }

	/** Returns true if the player was crouching before the dodge maneuver started. */
	UFUNCTION(BlueprintPure, Category = "Movement|Dodge")
	bool WasCrouchingPreDodge() const { return bWasCrouchingPreDodge; }

	/** Gets the owning MKHPlayerCharacter with proper valid checking. */
	UFUNCTION(BlueprintCallable, Category = "Movement|Dodge")
	AMKHPlayerCharacter* GetPlayerCharacter() const { return GetValidPlayerCharacter(); }

	// Inline Setters
	void SetIsDodging(bool bNewIsDodging) { bIsDodging = bNewIsDodging; }
	void SetDodgeSpeed(float NewDodgeSpeed) { DodgeSpeed = NewDodgeSpeed; }
	void SetInputInfluenceFactor(float NewFactor) { InputInfluenceFactor = FMath::Clamp(NewFactor, 0.0f, 1.0f); }
	void SetDodgeDirection(const FVector& NewDodgeDirection) { DodgeDirection = NewDodgeDirection; }
	void SetWasCrouchingPreDodge(bool bNewWasCrouching) { bWasCrouchingPreDodge = bNewWasCrouching; }

	// ============================================================
	// Protected / Internal Logic
	// ============================================================
protected:
	/** Retrieves a valid reference to the owning player character. */
	AMKHPlayerCharacter* GetValidPlayerCharacter() const;

private:
	/** RPC callback executed when the pre-dodge crouching state is updated. */
	UFUNCTION()
	void OnRep_bWasCrouchingPreDodge();

	/** Evaluates current input, checking either local Enhanced Input or remote replicated acceleration. */
	bool TryFetchMovementInput(bool& bOutHasInput, FVector& OutMovementInput) const;

	/** Calculates world-space input direction based on the character's forward/right vectors. */
	FVector CalculateLocalInputDirection(const FVector& MovementInput) const;

	/** Handles setting the dodge direction without blending, primarily for initial execution. */
	bool HandleInitialDodgeDirection(bool bHasInput, const FVector& MovementInput);

	/** Updates the current dodge direction by blending the initial direction with current input. */
	void UpdateBlendedDodgeDirection(bool bHasInput, const FVector& MovementInput);

	// ============================================================
	// Properties
	// ============================================================
public:
	/** Input action used to trigger a dodge maneuver. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> DodgeAction;

protected:
	/** Controls how much player input affects dodge direction. 0 = locked direction, 1 = full current input influence. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float InputInfluenceFactor = 0.49f;

	/** Movement speed during a dodge in cm/s. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", meta = (ClampMin = "100.0", ClampMax = "1000.0"))
	float DodgeSpeed = 650.0f;

	/** Whether the character is currently performing a dodge maneuver. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "State")
	bool bIsDodging = false;

	/** World-space dodge direction vector. */
	UPROPERTY(BlueprintReadOnly, Category = "State", meta=(AllowPrivateAccess="true"))
	FVector DodgeDirection = FVector::ZeroVector;

	/** World-space initial dodge direction captured at the start of the maneuver. */
	UPROPERTY(BlueprintReadOnly, Category = "State", meta=(AllowPrivateAccess="true"))
	FVector InitialDodgeDirection = FVector::ZeroVector;

	/** Tracks if the character was crouching before dodging, to restore crouch state conditionally. */
	UPROPERTY(ReplicatedUsing=OnRep_bWasCrouchingPreDodge, BlueprintReadOnly, Category = "State")
	bool bWasCrouchingPreDodge = false;

private:
	/** Cached reference to the character owning this component. */
	UPROPERTY()
	TObjectPtr<AMKHPlayerCharacter> OwnerPlayerCharacter;
};
