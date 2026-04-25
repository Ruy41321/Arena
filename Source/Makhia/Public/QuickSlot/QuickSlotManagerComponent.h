// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "QuickSlotManagerComponent.generated.h"

DECLARE_DELEGATE_OneParam(FQuickSlotActivatedSignature, const FGameplayTag& /*QuickSlotTag*/);

class UInventoryComponent;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UQuickSlotManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** Delegate invoked when a quick slot is activated through player input. */
	FQuickSlotActivatedSignature QuickSlotActivatedDelegate;

	/** Creates the quick slot manager component. */
	UQuickSlotManagerComponent();

	/** Assigns an item to a quick slot tag and updates weapon quick-equip tracking when needed. */
	void AddQuickSlot(const FGameplayTag& QuickSlotTag, int64 ItemID);

	/** Removes the slot entry associated with the provided item identifier. */
	void RemoveQuickSlot(int64 ItemID);

	/** Uses the quick slot identified by the provided gameplay tag. */
	UFUNCTION(BlueprintCallable, Category = "Quick Slot")
	void UseQuickSlot(const FGameplayTag& QuickSlotTag);

	/** Returns the item identifier currently bound to a quick slot tag, or 0 if unassigned. */
	int64 GetQuickSlotID(const FGameplayTag& QuickSlotTag) const;

	/**
	 * Attempts to equip the last weapon quick slot used while skipping activation callbacks.
	 * @return True if a valid weapon quick slot was found and used.
	 */
	bool TryEquipWeapon();

private:
	/** Applies the activation callback or restores activation gating after a forced use. */
	void HandleQuickSlotActivation(const FGameplayTag& QuickSlotTag);

	/** Returns the item identifier pointer mapped to a slot tag, or nullptr when not mapped. */
	const int64* FindItemIDForQuickSlot(const FGameplayTag& QuickSlotTag) const;

	/** Resolves the owner's inventory component through the inventory interface. */
	UInventoryComponent* ResolveInventoryComponent() const;

	/** Updates the tracked quick-equip weapon tag when the used slot belongs to the weapon category. */
	void UpdateQuickEquipWeaponSlotOnUse(const FGameplayTag& QuickSlotTag);

	/** Restores a fallback weapon quick slot after removing the currently tracked one. */
	void RefreshQuickEquipWeaponSlotTag();

	/** Stores the quick slot tag used for fast weapon equip actions. */
	FGameplayTag QuickEquipWeaponSlotTag;

	/** Maps quick slot gameplay tags to inventory item identifiers. */
	TMap<FGameplayTag, int64> QuickSlotTagMap;

	/** Prevents delegate activation during forced equip flows and restores on next use. */
	bool bCanTriggerActivation = true;
};
