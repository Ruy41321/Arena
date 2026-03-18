// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "EquipmentInstance.generated.h"

struct FEquipmentActorToSpawn;

/**
 * 
 */
UCLASS()
class ARENA_API UEquipmentInstance : public UObject
{
	GENERATED_BODY()
	
public:

	virtual void OnEquipped();
	virtual void OnUnEquipped();

	void SpawnEquipmentActors(const TArray<FEquipmentActorToSpawn>& ActorsToSpawn);
	void DestroySpawnedActors();

	TArray<AActor*> GetSpawnedActors();
	
private:
	
	UPROPERTY()
	TArray<AActor*> SpawnedActors;

	ACharacter* GetCharacter();

};
