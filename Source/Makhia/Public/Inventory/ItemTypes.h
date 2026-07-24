// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Equipment/EquipmentDefinition.h"
#include "ItemTypes.generated.h"

class UGameplayEffect;

USTRUCT(BlueprintType)
struct FEquipmentItemProps
{
	GENERATED_BODY()

	/** Equipment definition class used to instantiate and configure this equipment item. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UEquipmentDefinition> EquipmentClass = nullptr;
};

USTRUCT(BlueprintType)
struct FConsumableProps
{
	GENERATED_BODY()

	/** Gameplay Effect class applied when this consumable item is used. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UGameplayEffect> ItemEffectClass = nullptr;

	/** Level used when applying the consumable Gameplay Effect. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float ItemEffectLevel = 1.f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float CooldownTime = 0.f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTag CooldownTag = FGameplayTag();

};

USTRUCT(BlueprintType)
struct FMasterItemDefinition : public FTableRowBase
{
	GENERATED_BODY()

	/** Unique gameplay tag identifying the item type/category. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTag ItemTag = FGameplayTag();

	/** Localized text describing the item's gameplay purpose or lore. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FText ItemDescription = FText();

	/**
	 * Optional pre-defined rarity for this item.
	 * When set, equipment entries skip the random rarity roll and use this tier directly;
	 * when left empty, rarity is rolled from the rarity table as usual.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTag RarityTag = FGameplayTag();

	/** Current stack quantity of the item instance. */
	UPROPERTY(BlueprintReadOnly)
	int32 ItemQuantity = 0;

	/** Display name shown in UI and inventory views. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FText ItemName = FText();

	/** Icon texture displayed for this item in UI. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UTexture2D> Icon = nullptr;

	/** Consumable-specific configuration data for this item. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FConsumableProps ConsumableProps = FConsumableProps();

	/** Equipment-specific configuration data for this item. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FEquipmentItemProps EquipmentItemProps = FEquipmentItemProps();

};