// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Equipment/EquipmentTypes.h"
#include "ItemDisplayEntry.generated.h"

/**
 * Unified display-only struct used by the UI to represent both inventory and equipment items.
 * Does NOT participate in replication — it is built on-demand from FRPGInventoryEntry or FRPGEquipmentEntry.
 */
USTRUCT(BlueprintType)
struct FRPGItemDisplayEntry
{
	GENERATED_BODY()

	/** Tag that identifies the item (maps to FRPGInventoryEntry::ItemTag / FRPGEquipmentEntry::EntryTag). */
	UPROPERTY(BlueprintReadOnly)
	FGameplayTag ItemTag = FGameplayTag();

	/** Equipment slot this item occupies (empty for non-equipment / inventory-only items). */
	UPROPERTY(BlueprintReadOnly)
	FGameplayTag SlotTag = FGameplayTag();

	/** Rarity of the item. */
	UPROPERTY(BlueprintReadOnly)
	FGameplayTag RarityTag = FGameplayTag();

	/** Stack count (always 1 for equipped items). */
	UPROPERTY(BlueprintReadOnly)
	int32 Quantity = 0;

	/** Display name. */
	UPROPERTY(BlueprintReadOnly)
	FText ItemName = FText();

	/** Stat effects and abilities rolled on this item. */
	UPROPERTY(BlueprintReadOnly)
	FEquipmentEffectPackage EffectPackage = FEquipmentEffectPackage();

	/** Inventory-assigned unique ID (0 when coming from an equipment entry). */
	UPROPERTY(BlueprintReadOnly)
	int64 ItemID = 0;

	/** True when the source was an FRPGEquipmentEntry. */
	UPROPERTY(BlueprintReadOnly)
	bool bIsEquipped = false;
};

