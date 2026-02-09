// Copyright (c) 2025 Luigi Pennisi. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Player/MovementStateMachine/MovementStateTypes.h"
#include "PlayerAnimInstance.generated.h"

class APlayerCharacter;

UCLASS()
class ARENA_API UPlayerAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	FString GetCurrentStateMachineName();
	
protected:
    virtual void NativeBeginPlay() override;
    virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual void NativeUninitializeAnimation() override;
	
	/** Callback for when movement state changes */
	UFUNCTION()
	void OnMovementStateChanged(EMovementState OldState, EMovementState NewState);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	class APlayerCharacter* PlayerCharacter;

	// Locomotion properties for Blend Space
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Locomotion")
	float Speed;

	// Movement State Machine properties - now updated via observer pattern
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement State Machine")
	EMovementState CurrentMovementState;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement State Machine")
	EMovementState PreviousMovementState;

	// Crouching properties
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	float CrouchingRate = 1000.0f; // Rate at which the player crouches, can be adjusted in the editor

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	float CrouchingTransitionTime = 0.0f;

	// Helper function to check if states are valid (for Blueprint debugging)
	UFUNCTION(BlueprintPure, Category = "Movement State Machine")
	bool IsMovementStateDataValid() const;

	// Get movement state as string for debugging
	UFUNCTION(BlueprintPure, Category = "Movement State Machine")
	FString GetCurrentMovementStateString() const;

	UFUNCTION(BlueprintPure, Category = "Movement State Machine")
	FString GetPreviousMovementStateString() const;

private:
	/** Flag to track if we're subscribed to state change events */
	bool bIsSubscribedToStateChanges = false;

	/** Cached crouch transition target - updated only when state changes */
	float CrouchingTransitionTarget = 0.0f;

	/** Helper function to subscribe to movement state changes */
	void SubscribeToMovementStateChanges();

	/** Helper function to unsubscribe from movement state changes */
	void UnsubscribeFromMovementStateChanges();
};
