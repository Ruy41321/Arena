// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/MKHAbilityTypes.h"
#include "AbilitySystem/Abilities/MKHDamageAbility.h"
#include "MKHProjectileAbility.generated.h"

class AMKHProjectileBase;

/**
 * Damage ability that spawns projectile actors.
 *
 * The projectile to spawn is identified by ProjectileToSpawnTag (injected per grant via
 * UMKHAbilityGrantPayload) and resolved into FProjectileParams through the UProjectileInfo
 * data asset. Spawning is driven by the Event::SpawnProjectile gameplay event, typically
 * raised by an animation notify.
 *
 * The spawn follows the same locally-detected / server-authoritative pattern as the melee
 * hit pipeline: the notify is only reliable on the locally-controlled machine (owning client
 * for players, server for AI), so that machine listens for the event and computes the aim
 * point from its own view. When that machine is not the authority, the aim point is shipped
 * to the server as replicated target data; only the authority ever spawns the projectile
 * actor, which then replicates to every client.
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
	 * Activates the ability. Binds the spawn projectile event on the locally-controlled
	 * machine (the only one whose animation notifies are reliable) and the replicated aim
	 * point pipeline on the server proxy of remote players, which performs the actual spawn.
	 */
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/**
	 * Ends the ability after setting the default Character orientation and visibility of the weapon
	 */
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	
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

	/**
	 * Computes the world-space aim point for the projectile on the locally-controlled machine.
	 * Mirrors the WaitTargetData single-line-trace setup previously used in Blueprint
	 * (NoCollision trace profile, aim pitch affecting): the point AimTraceMaxRange units along
	 * the controller's view direction, re-projected from the avatar's location. Falls back to
	 * the avatar's forward vector when no player controller is available (AI).
	 *
	 * @return The absolute world-space aim point.
	 */
	FVector ComputeAimPoint() const;

	/**
	 * Reacts on this machine to the projectile being fired: stops following the camera
	 * orientation and hides the owning weapon (resolved lazily) until the ability ends.
	 */
	void HandleProjectileFiredReaction();

	// ==========================================
	// Replicated Aim Point (client -> server)
	// ==========================================

	/**
	 * Packages the locally-computed aim point and ships it to the server through the GAS
	 * replicated target data pipeline inside a scoped prediction window.
	 *
	 * @param AimPoint  Absolute world-space aim point computed on the owning client.
	 */
	void SendSpawnProjectileToServer(const FVector& AimPoint);

	/** Subscribes the server proxy to the replicated aim point sent by the owning client. */
	void BindServerSpawnTargetDataDelegate();

	/** Unsubscribes the server proxy from replicated target data and consumes leftovers. */
	void RemoveServerSpawnTargetDataDelegate();

	/**
	 * Server-side entry point for the replicated aim point. Consumes the payload and performs
	 * the authoritative projectile spawn towards the received location.
	 *
	 * @param DataHandle      The replicated target data batch.
	 * @param ApplicationTag  Optional application tag forwarded by the sender (unused).
	 */
	void OnServerSpawnTargetDataReceived(const FGameplayAbilityTargetDataHandle& DataHandle, FGameplayTag ApplicationTag);

	// ==========================================
	// Aiming Configuration
	// ==========================================

	/** Range (Unreal units) of the aim point along the controller's view direction; mirrors the Max Range of the Blueprint WaitTargetData trace. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true", ClampMin = "0"), Category = "Projectile Ability | Aiming")
	float AimTraceMaxRange = 999999.f;
};
