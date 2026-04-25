// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "EquipmentInstance.generated.h"

struct FEquipmentActorToSpawn;
class AEquipmentActor;
class ACharacter;

/**
 * Runtime object created per equipped item to manage spawned actors and equip lifecycle callbacks.
 */
UCLASS()
class MAKHIA_API UEquipmentInstance : public UObject
{
	GENERATED_BODY()
	
public:

	/** Called after this instance has been equipped and fully initialized. */
	virtual void OnEquipped();
	/** Called right before this instance is unequipped and removed. */
	virtual void OnUnEquipped();

	/** Spawns and attaches all actor specs defined by the equipment definition. */
	void SpawnEquipmentActors(const TArray<FEquipmentActorToSpawn>& ActorsToSpawn, float WeaponDamage);
	/** Destroys all actors spawned by this instance and clears runtime references. */
	void DestroySpawnedActors();

	/** Returns the runtime actors currently spawned by this equipment instance. */
	const TArray<TObjectPtr<AActor>>& GetSpawnedActors() const;
	
private:
	/** Spawns one actor from a spec and stores it if creation succeeds. */
	void SpawnActorFromSpecification(const FEquipmentActorToSpawn& ActorToSpawn, ACharacter* OwningCharacter, float WeaponDamage);
	/** Requests asynchronous class loading, then spawns the actor when loading completes. */
	void RequestAsyncSpawn(const FEquipmentActorToSpawn& ActorToSpawn, ACharacter* OwningCharacter, float WeaponDamage);
	/** Finalizes deferred actor spawn, attachment, and optional weapon damage initialization. */
	void FinalizeSpawnedActor(AEquipmentActor* SpawnedActor, ACharacter* OwningCharacter, const FName& AttachName, float WeaponDamage) const;
	/** Applies weapon damage only when the spawned actor is a weapon actor type. */
	static void ApplyWeaponDamageIfWeapon(AEquipmentActor* SpawnedActor, float WeaponDamage);
	/** Returns the owning character resolved from this object's outer hierarchy. */
	ACharacter* GetCharacter() const;
	
	/** Runtime array of actors spawned while this equipment instance is equipped. */
	UPROPERTY()
	TArray<TObjectPtr<AActor>> SpawnedActors;

};
