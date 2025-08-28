// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InputActionValue.h"
#include "SprintSystemComponent.generated.h"

class APlayerCharacter;
class UInputAction;
class UEnhancedInputComponent;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ARENA_API USprintSystemComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	USprintSystemComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Sets up input bindings for this component */
	UFUNCTION(BlueprintCallable, Category = "Sprint System", meta = (ToolTip = "Sets up sprint input bindings"))
	void SetupInput(UEnhancedInputComponent* EnhancedInputComponent);

	/** Handles sprint input - starts/stops sprinting */
	UFUNCTION(BlueprintCallable, Category = "Sprint System", meta = (ToolTip = "Handles sprint input and conditions"))
	void SprintPressed(const FInputActionValue& Value);

	/** Returns true if character is currently sprinting */
	UFUNCTION(BlueprintPure, Category = "Sprint System", meta = (ToolTip = "Check if character is sprinting"))
	bool IsSprinting() const { return bIsSprinting; }

	/** Returns true if sprint is currently interrupted */
	UFUNCTION(BlueprintPure, Category = "Sprint System", meta = (ToolTip = "Check if sprint is interrupted"))
	bool IsSprintInterrupted() const { return bSprintInterrupted; }

	/** Gets run/sprint speed */
	UFUNCTION(BlueprintPure, Category = "Sprint System", meta = (ToolTip = "Returns run speed in cm/s"))
	float GetRunSpeed() const { return RunSpeed; }

	/** Gets the owning PlayerCharacter */
	UFUNCTION(BlueprintCallable, Category = "Sprint System", meta = (ToolTip = "Returns the PlayerCharacter owner"))
	APlayerCharacter* GetPlayerCharacter() const { return GetValidPlayerCharacter(); }

	// Setters for external control
	void SetIsSprinting(bool NewIsSprinting) { bIsSprinting = NewIsSprinting; }
	void SetSprintInterrupted(bool NewSprintInterrupted) { bSprintInterrupted = NewSprintInterrupted; }
	void SetRunSpeed(float NewRunSpeed) { RunSpeed = NewRunSpeed; }

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
	/** Run/Sprint speed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprint System", 
		meta = (ToolTip = "Run speed in cm/s", ClampMin = "100.0", ClampMax = "1500.0"))
	float RunSpeed = 600.0f;

	/** Whether character is currently sprinting */
	UPROPERTY(BlueprintReadOnly, Category = "Sprint System", 
		meta = (ToolTip = "True if character is sprinting"))
	bool bIsSprinting = false;

	/** Whether sprint is currently interrupted */
	UPROPERTY(BlueprintReadOnly, Category = "Sprint System", 
		meta = (ToolTip = "True if sprint is interrupted"))
	bool bSprintInterrupted = true;

private:
	/** Cached PlayerCharacter reference */
	TObjectPtr<APlayerCharacter> OwnerPlayerCharacter;

public:
	/** Input action for sprint */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> SprintAction;
};
