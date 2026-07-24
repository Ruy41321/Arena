// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/MKHAbilityTypes.h"
#include "Equipment/EquipmentActor.h"
#include "MKHWeaponBase.generated.h"

struct FDamageEffectInfo;

/** Broadcast for every unique actor detected by the native melee scan during the current window. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMeleeHitDetected, const FHitResult&, HitResult);

/**
 * Base equipment actor for weapons, exposing trace/projection points and damage context for abilities.
 *
 * Provides a native melee hit scan: a timer-driven swept-sphere test sampled along the blade
 * segment (TraceStart -> TraceEnd). Each sample is swept from its position on the previous
 * scan step to its current position, so fast swings cannot tunnel through targets between
 * two steps. Detected actors are de-duplicated per scan window and reported through
 * OnMeleeHitDetected; damage routing/validation is the owning ability's responsibility.
 */
UCLASS()
class MAKHIA_API AMKHWeaponBase : public AEquipmentActor
{
	GENERATED_BODY()

public:

// ============================================================
// Lifecycle
// ============================================================

	/** Creates default scene points used for hit scan traces and projectile spawning. */
	AMKHWeaponBase();

// ============================================================
// Public Interface
// ============================================================

	/**
	 * Starts the native melee scan window. Clears the per-window hit set, caches the current
	 * blade endpoints and begins stepping at the requested frequency. Restarting an active
	 * scan resets the window (new swing).
	 *
	 * @param StepFrequency  Seconds between scan steps (clamped to a sane minimum).
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon | Melee Scan")
	void StartMeleeScan(float StepFrequency);

	/** Stops the native melee scan window and its driving timer. Safe to call when inactive. */
	UFUNCTION(BlueprintCallable, Category = "Weapon | Melee Scan")
	void StopMeleeScan();

	/** Returns true while a melee scan window is currently running. */
	UFUNCTION(BlueprintPure, Category = "Weapon | Melee Scan")
	bool IsMeleeScanActive() const;

	/** Fired once per unique actor detected during the current scan window. */
	UPROPERTY(BlueprintAssignable, Category = "Weapon | Melee Scan")
	FOnMeleeHitDetected OnMeleeHitDetected;

	/** Sets the runtime base damage value used by abilities reading from the weapon actor. */
	void SetWeaponDamage(float InDamage);

	/** Returns the runtime base damage value currently stored on the weapon actor. */
	float GetWeaponDamage() const;

	/** Sets the block stability percent value. */
	void SetBlockStabilityPercent(float InBlockStabilityPercent);

	/** Return the block stability percent value for this weapon. */
	float GetBlockStabilityPercent() const;

	/** Returns the projectile spawn position, falling back to actor location when missing. */
	FVector GetProjectileSpawnLocation() const;

protected:

// ============================================================
// Protected / Internal Logic
// ============================================================

	/**
	 * Executes one melee scan step: sweeps a sphere along the current blade segment and, for
	 * every blade sample, from its previous-step position to its current one. New actors are
	 * appended to HitActors and broadcast through OnMeleeHitDetected.
	 */
	void PerformMeleeScanStep();

	/**
	 * Runs a single swept-sphere query and reports every previously-unseen actor it touched.
	 *
	 * @param SweepStart  World-space start of the sweep.
	 * @param SweepEnd    World-space end of the sweep.
	 */
	void SweepAndReportHits(const FVector& SweepStart, const FVector& SweepEnd);

	/** Builds the list of actors the scan must ignore (weapon, owner chain, instigator). */
	void BuildScanIgnoreList(TArray<AActor*>& OutIgnoredActors) const;

// ============================================================
// Properties
// ============================================================

	/** Scene point used by projectile abilities as initial spawn origin. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Custom Values | Projectile")
	TObjectPtr<USceneComponent> ProjectileSpawnPoint;

	/** Hit scan trace start point (blade base). */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Custom Values | Hit Scan")
	TObjectPtr<USceneComponent> TraceStart;

	/** Hit scan trace end point (blade tip). */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Custom Values | Hit Scan")
	TObjectPtr<USceneComponent> TraceEnd;

	/** Actors already hit during the current scan window to avoid duplicate application. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Custom Values | Hit Scan")
	TArray<AActor*> HitActors;

	/** Timer handle driving repeated scan checks when continuous hit scan is active. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Custom Values | Hit Scan")
	FTimerHandle HitScanTimer;

	/** Radius used for sweep/overlap style hit scan checks. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Custom Values | Hit Scan")
	float HitScanRadius = 0.f;

	/** Object types the melee scan sweeps against (defaults to Pawn). */
	UPROPERTY(EditAnywhere, Category = "Custom Values | Hit Scan")
	TArray<TEnumAsByte<EObjectTypeQuery>> ScanObjectTypes;

	/** Number of sample points distributed along the blade segment for motion sweeps. */
	UPROPERTY(EditAnywhere, Category = "Custom Values | Hit Scan", meta = (ClampMin = "1", ClampMax = "10"))
	int32 NumBladeSamples = 3;

	/** When true, draws debug shapes for every scan sweep (development aid). */
	UPROPERTY(EditAnywhere, Category = "Custom Values | Hit Scan")
	bool bDrawDebugScan = false;

private:

	/** Base damage value assigned from the equipment definition when equipped. */
	float WeaponDamage = 0.f;

	/** Value for which the stamina damage is reduced (stam_damage * (1 - BlockStabilityPercent)) */
	float BlockStabilityPercent = 0.f;

	/** Blade base position captured on the previous scan step. */
	FVector PreviousBladeStart = FVector::ZeroVector;

	/** Blade tip position captured on the previous scan step. */
	FVector PreviousBladeEnd = FVector::ZeroVector;

	/** True while a melee scan window is running. */
	bool bMeleeScanActive = false;
};
