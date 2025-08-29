// Copyright (c) 2025 Luigi Pennisi. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "InputActionValue.h"
#include "../Utils/Utils.h"
#include "../Components/Dodge/DodgeSystemComponent.h"
#include "../Components/Crouch/CrouchSystemComponent.h"
#include "../Components/BasicMovement/BasicMovementComponent.h"
#include "../Components/Jump/JumpSystemComponent.h"
#include "../Components/Sprint/SprintSystemComponent.h"
#include "MovementStateMachine/MovementStateMachine.h"
#include "MovementStateMachine/MovementStateTypes.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Components/InputComponent.h"
#include "Components/CapsuleComponent.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Character.h"

// Forward declarations for Enhanced Input
class UInputMappingContext;
class UInputAction;
class UCameraComponent;
class USpringArmComponent;

#include "PlayerCharacter.generated.h"

UCLASS()
class ARENA_API APlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	APlayerCharacter();

	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	void UpdateMaxWalkSpeed();

	// Movement State Machine helper functions
	UFUNCTION(BlueprintPure, Category = "Movement State Machine")
	EMovementState GetCurrentMovementState() const;

	UFUNCTION(BlueprintPure, Category = "Movement State Machine")
	EMovementState GetPreviousMovementState() const;

	UFUNCTION(BlueprintCallable, Category = "Movement State Machine")
	bool TransitionToMovementState(EMovementState NewState, bool bForceTransition = false);

	UFUNCTION(BlueprintPure, Category = "Movement State Machine")
	FString GetCurrentMovementStateAsString() const;

	/** Get the movement state machine component */
	UFUNCTION(BlueprintPure, Category = "Movement State Machine")
	UMovementStateMachine* GetMovementStateMachine() const { return MovementStateMachine.Get(); }

	/** Subscribe to movement state change events using Blueprint delegate */
	UFUNCTION(BlueprintCallable, Category = "Movement State Machine")
	void SubscribeToMovementStateChanges(UObject* Subscriber, const FString& FunctionName);

	/** Unsubscribe from movement state change events */
	UFUNCTION(BlueprintCallable, Category = "Movement State Machine")
	void UnsubscribeFromMovementStateChanges(UObject* Subscriber);

protected:
	virtual void BeginPlay() override;

	virtual void Landed(const FHitResult& Hit) override;

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Dodge System")
	TObjectPtr<UDodgeSystemComponent> DodgeSystem;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Crouch System")
	TObjectPtr<UCrouchSystemComponent> CrouchSystem;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Basic Movement")
	TObjectPtr<UBasicMovementComponent> BasicMovementSystem;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Jump System")
	TObjectPtr<UJumpSystemComponent> JumpSystem;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Sprint System")
	TObjectPtr<USprintSystemComponent> SprintSystem;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Movement State Machine")
	TObjectPtr<UMovementStateMachine> MovementStateMachine;

private:
	
protected:
	// Camera
	UPROPERTY(BlueprintReadWrite, category = "Camera")
	TObjectPtr<UCameraComponent> Camera;
	UPROPERTY(BlueprintReadWrite, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	// Input actions and mapping context
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;
};