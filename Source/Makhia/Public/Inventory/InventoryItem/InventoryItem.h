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
class MAKHIA_API UInventoryItem : public UObject
{
	GENERATED_BODY()

public:

	// -----------------------------------------------------------------------
	// Initialisation
	// -----------------------------------------------------------------------

	/** Creates a UI item from an inventory entry. */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Item", meta = (WorldContext = "WorldContextObject"))
	static UInventoryItem* CreateFromInventoryEntry(UObject* WorldContextObject, const FRPGInventoryEntry& InEntry);

	/** Creates a UI item from an equipment entry. */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Item", meta = (WorldContext = "WorldContextObject"))
	static UInventoryItem* CreateFromEquipmentEntry(UObject* WorldContextObject, const FRPGEquipmentEntry& InEntry);

	/** Initializes this object from an inventory entry payload. */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Item")
	void InitFromInventoryEntry(const FRPGInventoryEntry& InEntry);

	/** Initializes this object from an equipment entry payload. */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Item")
	void InitFromEquipmentEntry(const FRPGEquipmentEntry& InEquipEntry);

	/** Copies all data from another UInventoryItem and broadcasts OnItemUpdatedDelegate. */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Item")
	void CopyFrom(const UInventoryItem* Other);


	// -----------------------------------------------------------------------
	// Core getters
	// -----------------------------------------------------------------------

	/** Returns the item gameplay tag. */
	UFUNCTION(BlueprintPure, Category = "Inventory|Item")
	FGameplayTag GetItemTag() const { return ItemTag; }

	/** Returns the equipment slot tag if available. */
	UFUNCTION(BlueprintPure, Category = "Inventory|Item")
	FGameplayTag GetSlotTag() const { return SlotTag; }

	/** Returns the item rarity tag. */
	UFUNCTION(BlueprintPure, Category = "Inventory|Item")
	FGameplayTag GetRarityTag() const { return RarityTag; }

	/** Returns the current stack quantity. */
	UFUNCTION(BlueprintPure, Category = "Inventory|Item")
	int32 GetQuantity() const { return Quantity; }

	/** Returns the localized display name. */
	UFUNCTION(BlueprintPure, Category = "Inventory|Item")
	FText GetItemName() const { return ItemName; }

	/** Returns the rolled effect package. */
	UFUNCTION(BlueprintPure, Category = "Inventory|Item")
	FEquipmentEffectPackage GetEffectPackage() const { return EffectPackage; }

	/** Returns the unique inventory item ID. */
	UFUNCTION(BlueprintPure, Category = "Inventory|Item")
	int64 GetItemID() const { return ItemID; }

	/** Returns whether the item is currently equipped. */
	UFUNCTION(BlueprintPure, Category = "Inventory|Item")
	bool IsEquipped() const { return bIsEquipped; }

	/** Returns whether the item is currently assigned to a quick slot. */
	UFUNCTION(BlueprintPure, Category = "Inventory|Item")
	bool IsQuickSlotted() const { return bIsQuickSlotted; }
	
	/** Returns the assigned quick slot tag, if any. */
	UFUNCTION(BlueprintPure, Category = "Inventory|Item")
	FGameplayTag GetQuickSlotTag() const { return QuickSlotTag; }
	
	// -----------------------------------------------------------------------
	// ItemDefinition-derived getters (cached on init)
	// -----------------------------------------------------------------------

	/** Returns the cached icon from the resolved item definition. */
	UFUNCTION(BlueprintPure, Category = "Inventory|Item")
	UTexture2D* GetIcon() const { return Icon; }

	/** Returns the cached weapon base damage resolved from equipment definition. */
	UFUNCTION(BlueprintPure, Category = "Inventory|Item")
	float GetWeaponDamage() const { return BaseWeaponDamage; }
	
	/** Returns the cached item description text. */
	UFUNCTION(BlueprintPure, Category = "Inventory|Item")
	FText GetDescription() const { return Description; }

	// -----------------------------------------------------------------------
	// Raw data (read-only)
	// -----------------------------------------------------------------------

	/** Tag that identifies the item (maps to FRPGInventoryEntry::ItemTag / FRPGEquipmentEntry::EntryTag). */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Item")
	FGameplayTag ItemTag = FGameplayTag();

	/** Equipment slot this item occupies (empty for non-equipment / inventory-only items). */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Item")
	FGameplayTag SlotTag = FGameplayTag();

	/** Rarity of the item. */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Item")
	FGameplayTag RarityTag = FGameplayTag();

	/** Stack count (always 1 for equipped items). */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Item")
	int32 Quantity = 0;

	/** Display name. */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Item")
	FText ItemName = FText();

	/** Stat effects and abilities rolled on this item. */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Item")
	FEquipmentEffectPackage EffectPackage = FEquipmentEffectPackage();

	/** Inventory-assigned unique ID (0 when coming from an equipment entry). */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Item")
	int64 ItemID = 0;

	/** True when this item is assigned to a quick slot. */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Item")
	bool bIsQuickSlotted = false;
	
	/** Quick slot tag currently associated with this item. */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Item")
	FGameplayTag QuickSlotTag = FGameplayTag();
	
	/** True when the source was an FRPGEquipmentEntry. */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Item")
	bool bIsEquipped = false;

	/** Item definition resolved from ItemTag. */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Item")
	FMasterItemDefinition ItemDefinition;

	/** Broadcasts whenever this object is refreshed from another payload/item. */
	UPROPERTY(BlueprintAssignable)
	FItemUpdatedSignature OnItemUpdatedDelegate;

private:

	/** Cached from ItemDefinition on init. */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Item", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTexture2D> Icon = nullptr;

	/** Cached from ItemDefinition on init. */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Item", meta = (AllowPrivateAccess = "true"))
	float BaseWeaponDamage = 1.f;
	
	/** Cached from ItemDefinition on init. */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Item", meta = (AllowPrivateAccess = "true"))
	FText Description = FText();

	/** Copies all serializable fields from another inventory item without broadcasting delegates. */
	void CopyCoreDataFrom(const UInventoryItem& Other);

	/** Resolves item definition data from the owning player's inventory component. */
	bool TryResolveItemDefinition(FMasterItemDefinition& OutItemDefinition) const;

	/** Updates Icon/Description/BaseWeaponDamage from the resolved ItemDefinition. */
	void CacheDerivedDataFromItemDefinition();

	/** Clears derived cached presentation fields when definition resolution fails. */
	void ResetDerivedData();

	/** Resolves ItemDefinition and updates all derived cached fields. */
	void ResolveItemDefinition();
};
