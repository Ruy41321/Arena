// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InputActionValue.h"
#include "../Utils/Utils.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Components/InputComponent.h"
#include "Components/CapsuleComponent.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"

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

// Functions section

public:
	APlayerCharacter();

	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:
	void AdjustCapsuleHeight(float DeltaTime, float TargetCapsuleHeight, float TargetMeshHeight);
	
	// Check if there's enough space above the character to uncrouch
	bool CanUncrouchSafely() const;

protected:
	virtual void BeginPlay() override;

	void MoveForward(const FInputActionValue& Value);
	void MoveRight(const FInputActionValue& Value);
	void Sprint(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);

	void JumpPressed(const FInputActionValue& Value);
	virtual void Landed(const FHitResult& Hit) override;
	void ResetLanding();

	void CrouchPressed(const FInputActionValue& Value);
	virtual void Crouch(bool bClientSimulation = false) override;
	virtual void UnCrouch(bool bClientSimulation = false) override;

// Properties section

public:
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool IsLanding;
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool IsCrouched;
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool IsCrouchingInProgress = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float WalkSpeed = 300.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float RunSpeed = 600.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float CrouchSpeed = 100.0f;

private:
	FTimerHandle LandingTimerHandle;
	bool SprintInterrupted = true;

protected:
	// Camera
	UPROPERTY(EditAnywhere, BlueprintReadWrite, category = "Camera")
	TObjectPtr<UCameraComponent> Camera;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	// Input actions and mapping context
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> MoveForwardAction;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> MoveRightAction;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> LookAction;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> SprintAction;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> JumpPressedAction;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> CrouchPressedAction;

	// Capsule height adjustment parameters when crouching
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crouching")
	float CrouchTargetHeight; // Target height when crouching
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crouching")
	float StandTargetHeight; // Target height when standing
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crouching")
	float HeightAdjustmentRate; // Speed adjustment factor for crouching

	// Mesh location adjustment parameters when crouching
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crouching")
	float CrouchMeshHeightOffset; // Offset for the mesh when crouching
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crouching")
	float StandMeshHeightOffset; // Offset for the mesh when standing
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crouching")
	float MeshOffsetAdjustmentRate; // Speed adjustment factor for mesh position

	// Collision check parameters for uncrouching
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crouching", meta = (ToolTip = "Extra safety margin (in cm) when checking for obstacles above"))
	float UncrouchSafetyMargin = 5.0f;

};