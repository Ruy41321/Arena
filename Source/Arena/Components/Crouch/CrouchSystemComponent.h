// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InputActionValue.h"
#include "CrouchSystemComponent.generated.h"

class APlayerCharacter;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ARENA_API UCrouchSystemComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UCrouchSystemComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Handles crouch input - toggles between crouch and uncrouch */
	UFUNCTION(BlueprintCallable, Category = "Crouch System", meta = (ToolTip = "Toggles crouch state"))
	void CrouchPressed(const FInputActionValue& Value);

	/** Start crouching */
	UFUNCTION(BlueprintCallable, Category = "Crouch System", meta = (ToolTip = "Initiates crouch"))
	void Crouch(bool bClientSimulation = false);

	/** Stop crouching */
	UFUNCTION(BlueprintCallable, Category = "Crouch System", meta = (ToolTip = "Stops crouch"))
	void UnCrouch(bool bClientSimulation = false);

	/** Checks if it's safe to uncrouch (no obstacles above) */
	UFUNCTION(BlueprintPure, Category = "Crouch System", meta = (ToolTip = "Check if uncrouch is safe"))
	bool CanUncrouchSafely() const;

	/** Adjusts capsule height and mesh position during crouch transition */
	void AdjustCapsuleHeight(float DeltaTime, float TargetCapsuleHeight, float TargetMeshHeight);

	/** Returns true if currently crouched */
	UFUNCTION(BlueprintPure, Category = "Crouch System", meta = (ToolTip = "Check if player is crouched"))
	bool IsCrouched() const { return bIsCrouched; }

	/** Returns true if crouch transition is in progress */
	UFUNCTION(BlueprintPure, Category = "Crouch System", meta = (ToolTip = "Check if crouch transition is active"))
	bool IsCrouchingInProgress() const { return bIsCrouchingInProgress; }

	/** Gets crouch movement speed */
	UFUNCTION(BlueprintPure, Category = "Crouch System", meta = (ToolTip = "Returns crouch speed in cm/s"))
	float GetCrouchSpeed() const { return CrouchSpeed; }

	/** Gets crouch target height */
	UFUNCTION(BlueprintPure, Category = "Crouch System", meta = (ToolTip = "Returns crouch target height"))
	float GetCrouchTargetHeight() const { return CrouchTargetHeight; }

	/** Gets stand target height */
	UFUNCTION(BlueprintPure, Category = "Crouch System", meta = (ToolTip = "Returns stand target height"))
	float GetStandTargetHeight() const { return StandTargetHeight; }

	/** Gets crouch mesh height offset */
	UFUNCTION(BlueprintPure, Category = "Crouch System", meta = (ToolTip = "Returns crouch mesh height offset"))
	float GetCrouchMeshHeightOffset() const { return CrouchMeshHeightOffset; }

	/** Gets stand mesh height offset */
	UFUNCTION(BlueprintPure, Category = "Crouch System", meta = (ToolTip = "Returns stand mesh height offset"))
	float GetStandMeshHeightOffset() const { return StandMeshHeightOffset; }

	// Setters for external access
	void SetIsCrouched(bool NewIsCrouched) { bIsCrouched = NewIsCrouched; }
	void SetIsCrouchingInProgress(bool NewIsCrouchingInProgress) { bIsCrouchingInProgress = NewIsCrouchingInProgress; }
	void SetCrouchSpeed(float NewCrouchSpeed) { CrouchSpeed = NewCrouchSpeed; }

	/** Gets the owning PlayerCharacter */
	UFUNCTION(BlueprintCallable, Category = "Crouch System", meta = (ToolTip = "Returns the PlayerCharacter owner"))
	APlayerCharacter* GetPlayerCharacter() const { return GetValidPlayerCharacter(); }

public:
	/** Movement speed during crouch */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crouch System", 
		meta = (ToolTip = "Crouch speed in cm/s", ClampMin = "50.0", ClampMax = "500.0"))
	float CrouchSpeed = 100.0f;

	/** Target capsule height when crouching */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crouch System", 
		meta = (ToolTip = "Target capsule height when crouched", ClampMin = "30.0", ClampMax = "120.0"))
	float CrouchTargetHeight = 65.0f;

	/** Target capsule height when standing */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crouch System", 
		meta = (ToolTip = "Target capsule height when standing", ClampMin = "60.0", ClampMax = "150.0"))
	float StandTargetHeight = 90.0f;

	/** Speed adjustment factor for capsule height changes */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crouch System", 
		meta = (ToolTip = "Rate of height adjustment", ClampMin = "100.0", ClampMax = "1000.0"))
	float HeightAdjustmentRate = 300.0f;

	/** Mesh Z offset when crouching */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crouch System", 
		meta = (ToolTip = "Mesh vertical offset when crouched"))
	float CrouchMeshHeightOffset = -60.0f;

	/** Mesh Z offset when standing */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crouch System", 
		meta = (ToolTip = "Mesh vertical offset when standing"))
	float StandMeshHeightOffset = -90.0f;

	/** Speed adjustment factor for mesh position changes */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crouch System", 
		meta = (ToolTip = "Rate of mesh position adjustment", ClampMin = "100.0", ClampMax = "1000.0"))
	float MeshOffsetAdjustmentRate = 300.0f;

	/** Extra safety margin when checking for obstacles above */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crouch System", 
		meta = (ToolTip = "Extra safety margin (in cm) when checking for obstacles above", ClampMin = "0.0", ClampMax = "20.0"))
	float UncrouchSafetyMargin = 5.0f;

protected:
	/** Whether currently crouched */
	UPROPERTY(BlueprintReadOnly, Category = "Crouch System", 
		meta = (ToolTip = "True if player is crouched"))
	bool bIsCrouched = false;

	/** Whether crouch transition is in progress */
	UPROPERTY(BlueprintReadOnly, Category = "Crouch System", 
		meta = (ToolTip = "True while crouch transition is active"))
	bool bIsCrouchingInProgress = false;

private:
	/** Cached PlayerCharacter reference */
	TObjectPtr<APlayerCharacter> OwnerPlayerCharacter;

	/** Gets valid PlayerCharacter with fallback */
	FORCEINLINE APlayerCharacter* GetValidPlayerCharacter() const;
};