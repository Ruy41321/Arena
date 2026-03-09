// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Equipment/EquipmentTypes.h"
#include "Inventory/ItemTypes.h"
#include "UObject/Object.h"
#include "InventoryItem.generated.h"

struct FRPGInventoryEntry;
struct FRPGEquipmentEntry;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FItemUpdatedSignature);

/**
 * Wrapper object that the UI uses to represent a single item — either from inventory or from an equipment slot.
 */
UCLASS(BlueprintType, Blueprintable)
class ARENA_API UInventoryItem : public UObject
{
	GENERATED_BODY()

public:

	// -----------------------------------------------------------------------
	// Initialisation
	// -----------------------------------------------------------------------

	UFUNCTION(BlueprintCallable, Category = "Inventory Item", meta = (WorldContext = "WorldContextObject"))
	static UInventoryItem* CreateFromInventoryEntry(UObject* WorldContextObject, const FRPGInventoryEntry& InEntry);

	UFUNCTION(BlueprintCallable, Category = "Inventory Item", meta = (WorldContext = "WorldContextObject"))
	static UInventoryItem* CreateFromEquipmentEntry(UObject* WorldContextObject, const FRPGEquipmentEntry& InEntry);

	UFUNCTION(BlueprintCallable, Category = "Inventory Item")
	void InitFromInventoryEntry(const FRPGInventoryEntry& InEntry);

	UFUNCTION(BlueprintCallable, Category = "Inventory Item")
	void InitFromEquipmentEntry(const FRPGEquipmentEntry& InEquipEntry);

	/** Copies all data from another UInventoryItem and broadcasts OnItemUpdatedDelegate. */
	UFUNCTION(BlueprintCallable, Category = "Inventory Item")
	void CopyFrom(const UInventoryItem* Other);


	// -----------------------------------------------------------------------
	// getters
	// -----------------------------------------------------------------------

	UFUNCTION(BlueprintPure, Category = "Inventory Item")
	FGameplayTag GetItemTag() const { return ItemTag; }

	UFUNCTION(BlueprintPure, Category = "Inventory Item")
	FGameplayTag GetSlotTag() const { return SlotTag; }

	UFUNCTION(BlueprintPure, Category = "Inventory Item")
	FGameplayTag GetRarityTag() const { return RarityTag; }

	UFUNCTION(BlueprintPure, Category = "Inventory Item")
	int32 GetQuantity() const { return Quantity; }

	UFUNCTION(BlueprintPure, Category = "Inventory Item")
	FText GetItemName() const { return ItemName; }

	UFUNCTION(BlueprintPure, Category = "Inventory Item")
	FEquipmentEffectPackage GetEffectPackage() const { return EffectPackage; }

	UFUNCTION(BlueprintPure, Category = "Inventory Item")
	int64 GetItemID() const { return ItemID; }

	UFUNCTION(BlueprintPure, Category = "Inventory Item")
	bool IsEquipped() const { return bIsEquipped; }

	UFUNCTION(BlueprintPure, Category = "Inventory Item")
	bool IsQuickSlotted() const { return bIsQuickSlotted; }
	
	UFUNCTION(BlueprintPure, Category = "Inventory Item")
	FGameplayTag GetQuickSlotTag() const { return QuickSlotTag; }
	
	// -----------------------------------------------------------------------
	// ItemDefinition-derived getters (cached on init)
	// -----------------------------------------------------------------------

	UFUNCTION(BlueprintPure, Category = "Inventory Item")
	UTexture2D* GetIcon() const { return Icon; }

	UFUNCTION(BlueprintPure, Category = "Inventory Item")
	FText GetDescription() const { return Description; }

	// -----------------------------------------------------------------------
	// Raw data (read-only)
	// -----------------------------------------------------------------------

	/** Tag that identifies the item (maps to FRPGInventoryEntry::ItemTag / FRPGEquipmentEntry::EntryTag). */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory Item")
	FGameplayTag ItemTag = FGameplayTag();

	/** Equipment slot this item occupies (empty for non-equipment / inventory-only items). */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory Item")
	FGameplayTag SlotTag = FGameplayTag();

	/** Rarity of the item. */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory Item")
	FGameplayTag RarityTag = FGameplayTag();

	/** Stack count (always 1 for equipped items). */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory Item")
	int32 Quantity = 0;

	/** Display name. */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory Item")
	FText ItemName = FText();

	/** Stat effects and abilities rolled on this item. */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory Item")
	FEquipmentEffectPackage EffectPackage = FEquipmentEffectPackage();

	/** Inventory-assigned unique ID (0 when coming from an equipment entry). */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory Item")
	int64 ItemID = 0;

	/** Property to define if this item is QuickSlotted or not. */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory Item")
	bool bIsQuickSlotted = false;
	
	UPROPERTY(BlueprintReadOnly, Category = "Inventory Item")
	FGameplayTag QuickSlotTag = FGameplayTag();
	
	/** True when the source was an FRPGEquipmentEntry. */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory Item")
	bool bIsEquipped = false;

	/** Item definition resolved from ItemTag. */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory Item")
	FMasterItemDefinition ItemDefinition;

	UPROPERTY(BlueprintAssignable)
	FItemUpdatedSignature OnItemUpdatedDelegate;

private:

	/** Cached from ItemDefinition on init. */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory Item", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTexture2D> Icon = nullptr;

	/** Cached from ItemDefinition on init. */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory Item", meta = (AllowPrivateAccess = "true"))
	FText Description = FText();

	/** Resolves ItemDefinition and caches Icon/Description. */
	void ResolveItemDefinition();
};
