// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "EquipmentRollLibrary.generated.h"

class UDataTable;
class UEquipmentDefinition;
class UEquipmentStatEffects;
struct FEquipmentStatEffectDefinition;
struct FEquipmentAbilityDefinition;
struct FRarityDefinition;

/**
 * Static library that handles all equipment roll logic: rarity selection,
 * passive stat rolling, and active ability rolling.
 */
UCLASS()
class ARENA_API UEquipmentRollLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/**
	 * Selects a rarity tag from the RarityTable using weighted probability.
	 * Returns the FRarityDefinition pointer for the selected row (nullptr on failure).
	 */
	static const FRarityDefinition* RollRarity(const UDataTable* RarityTable);

	/**
	 * Rolls passive stat effects for an equipment item.
	 * @param EquipmentCDO  The equipment CDO whose PossibleStatRolls define the candidate pool.
	 * @param StatData      The master stat data asset containing DataTables per tag category.
	 * @param NumStats      How many passive stats to attempt rolling (driven by rarity).
	 * @return Array of rolled stat effect definitions with their CurrentValue already set.
	 */
	static TArray<FEquipmentStatEffectDefinition> RollPassiveStats(
		const UEquipmentDefinition* EquipmentCDO,
		const UEquipmentStatEffects* StatData,
		int32 NumStats);

	/**
	 * Rolls active abilities for a weapon.
	 * @param EquipmentCDO  The equipment CDO whose PossibleAbilityRolls define the candidate pool.
	 * @param StatData      The master stat data asset containing DataTables per tag category.
	 * @param NumAbilities  How many active abilities to attempt rolling (driven by rarity).
	 * @return Array of rolled ability definitions.
	 */
	static TArray<FEquipmentAbilityDefinition> RollActiveAbilities(
		const UEquipmentDefinition* EquipmentCDO,
		const UEquipmentStatEffects* StatData,
		int32 NumAbilities);

	/**
	 * Resolves explicit ability tags into ability definitions.
	 * @param AbilityTags Container of ability tags to resolve.
	 * @param StatData    The master stat data asset containing DataTables per tag category.
	 * @return Array of resolved ability definitions (one per matching tag row).
	 */
	static TArray<FEquipmentAbilityDefinition> ResolveAbilitiesByTags(
		const FGameplayTagContainer& AbilityTags,
		const UEquipmentStatEffects* StatData);

private:

	/**
	 * Performs a weighted random selection over an array of weights.
	 * @param Weights  Array of positive weights (one per candidate).
	 * @return Index of the selected element, or -1 if the array is empty.
	 */
	static int32 WeightedRandomSelect(const TArray<float>& Weights);
};
