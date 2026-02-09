// Copyright (c) 2025 Luigi Pennisi. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Player/MovementStateMachine/MovementStateTypes.h"
#include "BasicMovementComponent.generated.h"

class APlayerCharacter;
class UInputAction;
class UEnhancedInputComponent;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ARENA_API UBasicMovementComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UBasicMovementComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Sets up input bindings for this component */
	UFUNCTION(BlueprintCallable, Category = "Basic Movement", meta = (ToolTip = "Sets up movement input bindings"))
	void SetupInput(UEnhancedInputComponent* EnhancedInputComponent);

	/** Handles forward/backward movement input */
	UFUNCTION(BlueprintCallable, Category = "Basic Movement", meta = (ToolTip = "Processes forward/backward movement"))
	void MoveForward(const FInputActionValue& Value);

	/** Handles left/right movement input */
	UFUNCTION(BlueprintCallable, Category = "Basic Movement", meta = (ToolTip = "Processes left/right movement"))
	void MoveRight(const FInputActionValue& Value);

	/** Handles camera look input */
	UFUNCTION(BlueprintCallable, Category = "Basic Movement", meta = (ToolTip = "Processes camera look input"))
	void Look(const FInputActionValue& Value);

	/** Called when movement input is completed */
	UFUNCTION(BlueprintCallable, Category = "Basic Movement", meta = (ToolTip = "Resets movement input when released"))
	void OnMovementInputCompleted(const FString& Axis);

	/** Updates movement velocity tracking */
	void UpdateMovementVelocity();

	/** Returns true if player has movement input */
	UFUNCTION(BlueprintPure, Category = "Basic Movement", meta = (ToolTip = "Check if player is providing movement input"))
	bool HasMovementInput() const { return bHasMovementInput; }

	/** Gets current movement input vector */
	UFUNCTION(BlueprintPure, Category = "Basic Movement", meta = (ToolTip = "Returns current movement input vector"))
	FVector GetCurrentMovementInput() const { return CurrentMovementInput; }

	/** Gets current movement velocity 2D */
	UFUNCTION(BlueprintPure, Category = "Basic Movement", meta = (ToolTip = "Returns current 2D velocity"))
	float GetMovementSpeed2D() const { return MovementSpeed2D; }

	/** Gets walk speed */
	UFUNCTION(BlueprintPure, Category = "Basic Movement", meta = (ToolTip = "Returns walk speed in cm/s"))
	float GetWalkSpeed() const { return WalkSpeed; }

	// Setters for external systems
	void SetHasMovementInput(bool bNewHasMovementInput) { bHasMovementInput = bNewHasMovementInput; }
	void SetCurrentMovementInput(const FVector& NewMovementInput) { CurrentMovementInput = NewMovementInput; }
	void SetCurrentMovementInputAxis(const FString& Axis, float Value);
	void SetWalkSpeed(float NewWalkSpeed) { WalkSpeed = NewWalkSpeed; }

	/** Gets the owning PlayerCharacter */
	UFUNCTION(BlueprintCallable, Category = "Basic Movement", meta = (ToolTip = "Returns the PlayerCharacter owner"))
	APlayerCharacter* GetPlayerCharacter() const { return GetValidPlayerCharacter(); }

	/** Gets valid PlayerCharacter with fallback */
	APlayerCharacter* GetValidPlayerCharacter() const;

public:
	/** Input actions for movement */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> MoveForwardAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> MoveRightAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	/** Mouse sensitivity for looking */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic Movement", 
		meta = (ToolTip = "Mouse sensitivity multiplier", ClampMin = "0.1", ClampMax = "5.0"))
	float MouseSensitivity = 1.0f;

	/** Invert Y axis for look input */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic Movement", 
		meta = (ToolTip = "Invert Y axis for camera look"))
	bool bInvertYAxis = false;

	/** Smoothing factor for movement input */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic Movement", 
		meta = (ToolTip = "Smoothing factor for movement transitions", ClampMin = "0.0", ClampMax = "1.0"))
	float MovementSmoothing = 0.1f;

protected:
	/** Walk speed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic Movement",
		meta = (ToolTip = "Walk speed in cm/s", ClampMin = "50.0", ClampMax = "1000.0"))
	float WalkSpeed = 300.0f;

	/** Whether player is currently providing movement input */
	UPROPERTY(BlueprintReadOnly, Category = "Basic Movement", 
		meta = (ToolTip = "True if player is providing movement input"))
	bool bHasMovementInput = false;

	/** Current movement input vector */
	UPROPERTY(BlueprintReadOnly, Category = "Basic Movement", 
		meta = (ToolTip = "Current movement input vector"))
	FVector CurrentMovementInput = FVector::ZeroVector;

	/** Current 2D movement speed */
	UPROPERTY(BlueprintReadOnly, Category = "Basic Movement", 
		meta = (ToolTip = "Current 2D movement speed"))
	float MovementSpeed2D = 0.0f;

private:
	/** Cached PlayerCharacter reference */
	TObjectPtr<APlayerCharacter> OwnerPlayerCharacter;
};
