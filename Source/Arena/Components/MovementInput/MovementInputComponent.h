// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InputActionValue.h"
#include "MovementInputComponent.generated.h"

class APlayerCharacter;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ARENA_API UMovementInputComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UMovementInputComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Handles forward/backward movement input */
	UFUNCTION(BlueprintCallable, Category = "Movement Input", meta = (ToolTip = "Processes forward/backward movement"))
	void MoveForward(const FInputActionValue& Value);

	/** Handles left/right movement input */
	UFUNCTION(BlueprintCallable, Category = "Movement Input", meta = (ToolTip = "Processes left/right movement"))
	void MoveRight(const FInputActionValue& Value);

	/** Handles camera look input */
	UFUNCTION(BlueprintCallable, Category = "Movement Input", meta = (ToolTip = "Processes camera look input"))
	void Look(const FInputActionValue& Value);

	/** Called when movement input is completed */
	UFUNCTION(BlueprintCallable, Category = "Movement Input", meta = (ToolTip = "Resets movement input when released"))
	void OnMovementInputCompleted(const FString& Axis);

	/** Updates movement velocity tracking */
	void UpdateMovementVelocity();

	/** Updates the character's maximum walk speed based on current movement state */
	UFUNCTION(BlueprintCallable, Category = "Movement Input", meta = (ToolTip = "Updates max walk speed based on dodge, crouch, and sprint states"))
	void UpdateMaxWalkSpeed();

	/** Returns true if player has movement input */
	UFUNCTION(BlueprintPure, Category = "Movement Input", meta = (ToolTip = "Check if player is providing movement input"))
	bool HasMovementInput() const { return bHasMovementInput; }

	/** Gets current movement input vector */
	UFUNCTION(BlueprintPure, Category = "Movement Input", meta = (ToolTip = "Returns current movement input vector"))
	FVector GetCurrentMovementInput() const { return CurrentMovementInput; }

	/** Gets current movement velocity 2D */
	UFUNCTION(BlueprintPure, Category = "Movement Input", meta = (ToolTip = "Returns current 2D velocity"))
	float GetMovementSpeed2D() const { return MovementSpeed2D; }

	// Setters for external systems
	void SetHasMovementInput(bool bNewHasMovementInput) { bHasMovementInput = bNewHasMovementInput; }
	void SetCurrentMovementInput(const FVector& NewMovementInput) { CurrentMovementInput = NewMovementInput; }
	void SetCurrentMovementInputAxis(const FString& Axis, float Value);

	/** Gets the owning PlayerCharacter */
	UFUNCTION(BlueprintCallable, Category = "Movement Input", meta = (ToolTip = "Returns the PlayerCharacter owner"))
	APlayerCharacter* GetPlayerCharacter() const { return GetValidPlayerCharacter(); }

public:
	/** Mouse sensitivity for looking */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement Input", 
		meta = (ToolTip = "Mouse sensitivity multiplier", ClampMin = "0.1", ClampMax = "5.0"))
	float MouseSensitivity = 1.0f;

	/** Invert Y axis for look input */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement Input", 
		meta = (ToolTip = "Invert Y axis for camera look"))
	bool bInvertYAxis = false;

	/** Smoothing factor for movement input */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement Input", 
		meta = (ToolTip = "Smoothing factor for movement transitions", ClampMin = "0.0", ClampMax = "1.0"))
	float MovementSmoothing = 0.1f;

protected:
	/** Whether player is currently providing movement input */
	UPROPERTY(BlueprintReadOnly, Category = "Movement Input", 
		meta = (ToolTip = "True if player is providing movement input"))
	bool bHasMovementInput = false;

	/** Current movement input vector */
	UPROPERTY(BlueprintReadOnly, Category = "Movement Input", 
		meta = (ToolTip = "Current movement input vector"))
	FVector CurrentMovementInput = FVector::ZeroVector;

	/** Current 2D movement speed */
	UPROPERTY(BlueprintReadOnly, Category = "Movement Input", 
		meta = (ToolTip = "Current 2D movement speed"))
	float MovementSpeed2D = 0.0f;

private:
	/** Cached PlayerCharacter reference */
	TObjectPtr<APlayerCharacter> OwnerPlayerCharacter;

	/** Gets valid PlayerCharacter with fallback */
	FORCEINLINE APlayerCharacter* GetValidPlayerCharacter() const;
};
