// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/MKHAbilityTypes.h"
#include "Equipment/EquipmentActor.h"
#include "MKHWeaponBase.generated.h"

struct FDamageEffectInfo;
/**
 * Base equipment actor for weapons, exposing trace/projection points and damage context for abilities.
 */
UCLASS()
class MAKHIA_API AMKHWeaponBase : public AEquipmentActor
{
	GENERATED_BODY()
	
public:
	
	/** Creates default scene points used for hit scan traces and projectile spawning. */
	AMKHWeaponBase();

	/** Blueprint event used to execute a weapon-specific hit scan implementation. */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void HitScan();
	
	/** Blueprint event fired when hit scan begins with prepared damage context. */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void HitScanStart(FDamageEffectInfo DamageEffectInfo);
	
	/** Blueprint event fired when hit scan ends. */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void HitScanEnd();
	
	/** Sets the runtime base damage value used by abilities reading from the weapon actor. */
	void SetWeaponDamage(float InDamage);
	
	/** Returns the runtime base damage value currently stored on the weapon actor. */
	float GetWeaponDamage() const;
	
	/** Returns the projectile spawn position, falling back to actor location when missing. */
	FVector GetProjectileSpawnLocation() const;
	
protected:
	
	/** Scene point used by projectile abilities as initial spawn origin. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Custom Values | Projectile")
	TObjectPtr<USceneComponent> ProjectileSpawnPoint;
	
	/** Hit scan trace start point used by Blueprint hit scan logic. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Custom Values | Hit Scan")
	TObjectPtr<USceneComponent> TraceStart;
	
	/** Hit scan trace end point used by Blueprint hit scan logic. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Custom Values | Hit Scan")
	TObjectPtr<USceneComponent> TraceEnd;
	
	/** Cached damage context consumed by Blueprint hit scan events. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Custom Values | Hit Scan")
	FDamageEffectInfo CachedDamageEffectInfo;
	
	/** Actors already hit during the current scan window to avoid duplicate application. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Custom Values | Hit Scan")
	TArray<AActor*> HitActors;
	
	/** Timer handle driving repeated scan checks when continuous hit scan is active. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Custom Values | Hit Scan")
	FTimerHandle HitScanTimer;
	
	/** Radius used for sweep/overlap style hit scan checks. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Custom Values | Hit Scan")
	float HitScanRadius = 0.f;

private:
	
	/** Base damage value assigned from the equipment definition when equipped. */
	float WeaponDamage = 0.f;
};
