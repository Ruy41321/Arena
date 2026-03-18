// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameplayTagContainer.h"
#include "EquipmentDefinition.generated.h"

class UEquipmentInstance;
class AEquipmentActor;

USTRUCT()
struct FEquipmentActorToSpawn
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	TSoftClassPtr<AEquipmentActor> EquipmentClass = nullptr;

	UPROPERTY(EditDefaultsOnly)
	FName AttachName = FName();
};

/**
 * 
 */
UCLASS(BlueprintType, Blueprintable)
class ARENA_API UEquipmentDefinition : public UObject
{
	GENERATED_BODY()
	
public:

	UPROPERTY(EditDefaultsOnly, Category = "Custom Values | Info")
	FGameplayTag ItemTag;

	UPROPERTY(EditDefaultsOnly, Category = "Custom Values | Info")
	FGameplayTag SlotTag;


	UPROPERTY(EditDefaultsOnly, Category = "Custom Values | Info")
	TSubclassOf<UEquipmentInstance> InstanceType;

	UPROPERTY(EditDefaultsOnly, Category = "Custom Values | Stats")
	FGameplayTagContainer PossibleStatRolls;

	UPROPERTY(EditDefaultsOnly, Category = "Custom Values | Actors")
	TArray<FEquipmentActorToSpawn> ActorsToSpawn;

	UPROPERTY(EditDefaultsOnly, Category = "Custom Values | Ability")
	FGameplayTagContainer PossibleAbilityRolls;
	
	UPROPERTY(EditDefaultsOnly, Category = "Custom Values | Ability")
	FGameplayTagContainer BasicAbilitiesGranted;
};
