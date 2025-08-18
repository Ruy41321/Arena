// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "../Player/PlayerCharacter.h"
#include "../Utils/Utils.h"
#include "PlayerAnimInstance.generated.h"

UCLASS()
class ARENA_API UPlayerAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
protected:
    virtual void NativeBeginPlay() override;
    virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	class APlayerCharacter* PlayerCharacter;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	float Speed;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	bool IsJumping;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	bool IsFalling;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	FVector Velocity;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	bool IsLanding;

	// Crouching properties

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	float CrouchingRate = 1000.0f; // Rate at which the player crouches, can be adjusted in the editor

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	float CrouchingTransitionTime = 0.0f;
};
