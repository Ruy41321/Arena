// Copyright (c) 2025 Luigi Pennisi. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/Engine.h"
#include "TimerManager.h"
#include "InputActionValue.h"
#include "Engine/HitResult.h"
#include "JumpSystemComponent.generated.h"

// Forward declaration
class APlayerCharacter;
class UInputAction;
class UEnhancedInputComponent;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ARENA_API UJumpSystemComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UJumpSystemComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Sets up input bindings for this component */
	UFUNCTION(BlueprintCallable, Category = "Jump System", meta = (ToolTip = "Sets up jump input bindings"))
	void SetupInput(UEnhancedInputComponent* EnhancedInputComponent);

	/** Handles jump input - jumps if conditions are met, uncrouches if crouched */
	UFUNCTION(BlueprintCallable, Category = "Jump System", meta = (ToolTip = "Handles jump input and conditions"))
	void JumpPressed(const FInputActionValue& Value);

	/** Handles landing event and sets landing state */
	UFUNCTION(BlueprintCallable, Category = "Jump System", meta = (ToolTip = "Called when character lands"))
	void OnLanded(const FHitResult& Hit);

	/** Returns true if character is currently in landing state */
	UFUNCTION(BlueprintPure, Category = "Jump System", meta = (ToolTip = "Check if character is landing"))
	bool IsLanding() const { return bIsLanding; }

	/** Resets landing state */
	UFUNCTION(BlueprintCallable, Category = "Jump System", meta = (ToolTip = "Resets landing state"))
	void ResetLanding();

	/** Gets the owning PlayerCharacter */
	UFUNCTION(BlueprintCallable, Category = "Jump System", meta = (ToolTip = "Returns the PlayerCharacter owner"))
	APlayerCharacter* GetPlayerCharacter() const { return GetValidPlayerCharacter(); }

	// Setters for external control
	void SetIsLanding(bool NewIsLanding) { bIsLanding = NewIsLanding; }
	void SetLandingResetTime(float NewLandingResetTime) { LandingResetTime = NewLandingResetTime; }

protected:
	/** Time in seconds before landing state is reset */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jump System", 
		meta = (ToolTip = "Time to wait before resetting landing state", ClampMin = "0.0", ClampMax = "1.0"))
	float LandingResetTime = 0.1f;

	/** Whether character is currently in landing state */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Jump System", 
		meta = (ToolTip = "True while character is landing"))
	bool bIsLanding = false;

private:
	/** Timer for resetting landing state */
	FTimerHandle LandingTimerHandle;

	/** Cached PlayerCharacter reference */
	TObjectPtr<APlayerCharacter> OwnerPlayerCharacter;

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
	
public:
	/** Input action for jump */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> JumpPressedAction;
};
