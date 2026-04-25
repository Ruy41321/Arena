// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/MKHAbilityTypes.h"
#include "AbilitySystem/Abilities/MKHDamageAbility.h"
#include "MKHProjectileAbility.generated.h"

class AMKHProjectileBase;
/**
 * 
 */
UCLASS()
class MAKHIA_API UMKHProjectileAbility : public UMKHDamageAbility
{
	GENERATED_BODY()
	
public:

	// ==========================================
	// Overrides
	// ==========================================

	/** 
	 * Activates the ability and binds the spawn projectile listener.
	 */
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	
	/**
	 * Called when the ability is granted, allowing cache configurations.
	 * 
	 * @param ActorInfo 
	 * @param Spec 
	 */
	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;

	// ==========================================
	// Projectile Configuration
	// ==========================================

	/** Tag identifying the projectile data from the mapped information. */
	UPROPERTY(BlueprintReadOnly, Category = "Projectile Ability | Setup")
	FGameplayTag ProjectileToSpawnTag;

	// ==========================================
	// Events
	// ==========================================

	/**
	 * Natively triggers logic when the spawn event is received via gameplay tags.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Projectile Ability | Events")
	void OnSpawnProjectileEvent(FGameplayEventData Payload);
	virtual void OnSpawnProjectileEvent_Implementation(FGameplayEventData Payload);
	
	/**
	 * Triggered when the spawned projectile is destroyed.
	 * @param DestroyedActor The actor instance that has been destroyed.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Projectile Ability | Events")
	void OnProjectileDestroyed(AActor* DestroyedActor);
	virtual void OnProjectileDestroyed_Implementation(AActor* DestroyedActor);
	
private:
	
	// ==========================================
	// Internal State & Variables
	// ==========================================

	/** A cached reference to the Avatar Actor currently invoking the ability. */
	UPROPERTY()
	TObjectPtr<AActor> AvatarActorFromInfo;
	
	/** Captured definition structure for the configured projectile. */
	UPROPERTY(BlueprintReadOnly, Category = "Projectile Ability | Internal", meta = (AllowPrivateAccess = "true"))
	FProjectileParams CurrentProjectileParams;
	
	/** Active reference pointing to the currently spawned projectile. */
	UPROPERTY(BlueprintReadOnly, Category = "Projectile Ability | Internal", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<AMKHProjectileBase> SpawnedProjectile = nullptr;

	// ==========================================
	// Spawning Logic
	// ==========================================

	/**
	 * Executes the physical instantiation of the Projectile actor aligned towards TargetLocation.
	 * @param TargetLocation Vector destination used to rotate the projectile orientation.
	 */
	UFUNCTION(BlueprintCallable, Category = "Projectile Ability | Logic")
	void SpawnProjectile(const FVector& TargetLocation);
	
	/** Starts listening for event payloads commanding to execute logic. */
	void BindSpawnProjectileEvent();
	
	/** Resolves the absolute world coordinate for spawning the projectile. */
	FVector GetSpawnLocation() const;
};
