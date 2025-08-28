// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InputActionValue.h"
#include "../Utils/Utils.h"
#include "../Components/Dodge/DodgeSystemComponent.h"
#include "../Components/Crouch/CrouchSystemComponent.h"
#include "../Components/BasicMovement/BasicMovementComponent.h"
#include "../Components/Jump/JumpSystemComponent.h"
#include "../Components/Sprint/SprintSystemComponent.h"

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